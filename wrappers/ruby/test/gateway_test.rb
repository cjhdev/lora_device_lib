require 'minitest/autorun'
require 'ldl'
require 'socket'
require 'timeout'
require 'securerandom'

class TestGateway < Minitest::Test

  include LDL

  attr_reader :s

  def rx_message
    msg = s.recvfrom(2048, 0)
    Semtech::Message.decode msg.first
  end

  def setup

    Thread.abort_on_exception = true

    @s = UDPSocket.new
    s.bind('localhost', 0)

    logger = Logger.new(STDOUT)
    logger.level = Logger::DEBUG
    logger.formatter = LDL::LOG_FORMATTER

    # - talk to localhost on the port we are listening to (above)
    # - shorten keepalive_interval so we can test fast
    # - shorten status_interval so we can test fast
    @scenario = Scenario.new(
      logger: logger,
      host: 'localhost',
      port: s.local_address.ip_port,
      keepalive_interval: 1,
      status_interval: 1
    )

    @scenario.start

    sleep 0.1

  end

  def gw
    @scenario.gw
  end

  def test_expect_keep_alive_on_start

    Timeout::timeout 1 do

      loop do

        msg = rx_message

        if msg.kind_of? Semtech::PullData
          break
        end

      end

    end

  end

  def test_expect_keep_alive_on_interval

    count = 0

    begin

      Timeout::timeout (gw.keepalive_interval + 0.5) do

        loop do

          msg = rx_message()

          if msg.kind_of? Semtech::PullData

            count += 1

          end

        end

      end

    rescue
    end

    # should have two keep alives
    assert count == 2

  end

  def test_upstream

    m1 = {
      :data => "hello world",
      :freq => 0,
      :sf => 7,
      :bw => 125,
      :air_time => 42
    }

    m2 = {
    }

    @scenario.broker.publish(m1, "tx_begin")
    @scenario.broker.publish(m2, "tx_end")

    Timeout::timeout 2 do

      loop do

        msg = rx_message

        if msg.kind_of? Semtech::PushData and not msg.rxpk.empty? and msg.rxpk.first.data == 'hello world'
          break
        end

      end

    end

  end

  def test_downstream_immediate

    q = Queue.new

    @scenario.broker.subscribe "tx_begin" do |msg|
      q.push msg
    end

    @scenario.broker.subscribe "tx_end" do |msg|
      q.push msg
    end

    txpk = Semtech::TXPacket.new(data: "hello world", imme: true)

    from = nil

    # wait for PullData
    Timeout::timeout 1 do

      loop do

        msg, from = s.recvfrom(2048, 0)
        msg = Semtech::Message.decode msg

        if msg.kind_of? Semtech::PullData

          break

        end

      end

    end

    s.send(Semtech::PullResp.new(txpk: txpk, token: 42).encode, 0, 'localhost', from[1])

    m1 = nil
    m2 = nil

    Timeout::timeout 2 do

      m1 = q.pop
      m2 = q.pop

    end

    assert m1[:eui] == m2[:eui]

    assert m1[:data] == 'hello world'

  end

  def teardown

    @scenario.stop
    @s.close
    Thread.abort_on_exception = false

  end

end
