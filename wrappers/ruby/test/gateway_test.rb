require 'minitest/autorun'
require 'ldl'
require 'socket'
require 'timeout'
require 'securerandom'

describe "GatewayTest" do

  let(:broker){ SmallEvent::Broker.new }
  let(:clock){ LDL::Clock.new(logger: logger) }
  let(:logger){ Logger.new(STDOUT) }
  let(:s){ UDPSocket.new }
  let(:q){ Queue.new }

  attr_reader :gw

  before do

    Thread.abort_on_exception = true

    s.bind('localhost', 0)

    logger.level = Logger::DEBUG
    logger.formatter = LDL::LOG_FORMATTER

    @gw = LDL::Gateway.new(broker, clock,
      logger: logger,
      host: 'localhost',
      port: s.local_address.ip_port,
      keepalive: 1
    )

    clock.start
    @gw.start

  end

  after do
    @gw.stop
    clock.stop
    s.close
    Thread.abort_on_exception = false
  end

  def rx_message
    msg = s.recvfrom(2048, 0)
    LDL::Semtech::Message.decode msg.first
  end

  it "sends PullData (keep alive) on start" do

    Timeout::timeout 1 do

      loop do

        msg = rx_message

        break if msg.kind_of? LDL::Semtech::PullData

      end

    end

  end

  it "sends PullData (keep alive) on regular interval" do

    count = 0

    begin

      Timeout::timeout ((2 * gw.keepalive) + (gw.keepalive / 2)) do

        loop do

          msg = rx_message()

          if msg.kind_of? LDL::Semtech::PullData

            count += 1

          end

        end

      end

    rescue
    end

    assert [2,3].include?(count)

  end

  it "forwards rx radio packets upstream" do

    data = SecureRandom.bytes(5)

    m1 = {
      data: data,
      freq: 868000000,
      sf: 7,
      bw: 125,
      air_time: 42,
      power: 10,
      reliability: 1.0,
      time: 0,
      ticks: 0
    }

    m2 = {
    }

    broker.publish(m1, "tx_begin")
    broker.publish(m2, "tx_end")

    Timeout::timeout 2 do

      loop do

        msg = rx_message

        case msg
        when LDL::Semtech::PushData
          break if msg.rxpk.first.data == data
        end

      end

    end

  end

  it "transmits tx radio packets immediately" do

    from = nil

    # wait for PullData
    Timeout::timeout 1 do

      loop do

        msg, from = s.recvfrom(2048, 0)
        msg = LDL::Semtech::Message.decode msg

        break if msg.kind_of? LDL::Semtech::PullData

      end

    end

    broker.subscribe("tx_begin") { |msg| q.push msg }
    broker.subscribe("tx_end") { |msg| q.push msg }

    txpk = LDL::Semtech::TXPacket.new(data: SecureRandom.bytes(5), imme: true)

    s.send(LDL::Semtech::PullResp.new(txpk: txpk, token: 42).encode, 0, 'localhost', from[1])

    m1 = nil
    m2 = nil

    Timeout::timeout 2 do

      m1 = q.pop
      m2 = q.pop

    end

    assert m1[:eui] == m2[:eui]

    assert m1[:data] == txpk.data

  end

end
