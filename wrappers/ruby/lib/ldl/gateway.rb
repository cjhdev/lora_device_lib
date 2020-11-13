require 'socket'

module LDL

  class Gateway

    include LoggerMethods
    include RadioModel

    MAX_TOKENS = 100

    # maximum allowable advance send scheduling
    MAX_ADVANCE_TIME = 10

    attr_reader :eui

    # shorter name assigned to make the log easier to read
    attr_reader :name

    # GPS latitude of the gateway in degree (float, N is +)
    attr_reader :lati

    # GPS latitude of the gateway in degree (float, E is +)
    attr_reader :long

    # GPS altitude of the gateway in meter RX (integer)
    attr_reader :alti

    # Number of radio packets received (unsigned integer)
    attr_reader :rxnb

    # Number of radio packets received with a valid PHY CRC
    attr_reader :rxok

    # Number of radio packets forwarded (unsigned integer)
    attr_reader :rxfw

    # Number of downlink datagrams received (unsigned integer)
    attr_reader :dwnb

    # Number of packets emitted (unsigned integer)
    attr_reader :txnb

    attr_reader :host
    attr_reader :port

    # @return [Integer] seconds
    attr_reader :keepalive_interval

    def broker
      @scenario.broker
    end

    attr_reader :gain, :location

    def increment_token
      @token += 1
      self
    end

    def initialize(scenario, **opts)

      @disable_log = true

      @scenario = scenario

      if opts[:gw_eui]
        @eui = EUI.new(opts[:gw_eui])
      else
        @eui = EUI.new(SecureRandom.bytes(8))
      end

      @tokens = []

      # use as message token, increment for each message sent
      @token = 0

      @gain = opts[:gain]||0
      @lati = opts[:lati]||51.4576
      @long = opts[:long]||0.9705
      @alti = opts[:alti]||61
      @port = opts[:port]||4002
      @host = opts[:host]||"localhost"
      @keepalive_interval = opts[:keepalive_interval]||10
      @name = opts[:name]||@eui.to_s("")
      @location = opts[:location]||Location.new(0,0)

      @log_header = "GATEWAY(#{name})"

      @q = Queue.new

      # worker thread
      @worker = Thread.new do

        # nothing old
        @q.clear

        # start the keep alive push
        @q.push({:task => :keepalive})

        buffer = []

        tx_begin = broker.subscribe "tx_begin" do |m1|

          # don't listen to our own stuff
          if m1[:station] != self
            buffer.push m1
          end

        end

        tx_end = broker.subscribe "tx_end" do |m2|

          if m1 = buffer.detect{ |v| v[:station] == m2[:station] }

            log_debug{"received frame"}

            @q.push(
              {
                :task => :upstream,
                :rxpk => Semtech::RXPacket.new(
                  tmst: tmst,
                  freq: m1[:freq].to_f / 1000000.0,
                  data: m1[:data],
                  datr: "SF#{m1[:sf]}BW#{m1[:bw]/1000}",
                  rssi: -rand(10..50),
                  lsnr: rand(-1..10),
                  rfch: 0
                )
              }
            )

            buffer.delete m1

          end

        end

        s = UDPSocket.new

        s.connect(host, port)

        Thread.new do

          begin

            loop do

              reply = s.recvfrom(1024, 0)

              @q.push({:task => :downstream, :data => reply.first})

            end

          rescue
            log_debug{"could not recv"}
          end

        end

        broker.publish({:station => self}, 'up')

        loop do

          job = @q.pop

          # stop the gateway by closing the socket and breaking this loop
          if job[:task] == :stop
            s.close # will cause exception for rx thread waiting on socket
            break
          end

          @scenario.clock.call do

            case job[:task]
            # perform a keep alive
            when :keepalive

              m = Semtech::PullData.new(token: @token, eui: @eui)

              add_token @token

              increment_token

              # schedule keep alive: note this will probably fire once after we stop this thread
              @scenario.clock.call_in(keepalive_interval * ExtMAC::TICKS_PER_SECOND) do

                  @q.push({:task => :keepalive})

              end

              log_debug{"#{self}"}
              log_debug{"sending PullData (keep alive) #{self.disable_log}"}

              s.write(m.encode)

            # send upstream
            when :upstream

              m = Semtech::PushData.new(token: @token, eui: @eui, rxpk: [job[:rxpk]])

              add_token @token

              increment_token

              log_debug{"sending PushData (forward)"}

              s.write(m.encode)

            # message received from network
            when :downstream

              begin
                msg = Semtech::Message.decode(job[:data])

                log_debug{"received #{msg.class.name.split("::").last}"}

                case msg.class
                when Semtech::PushAck, Semtech::PullAck

                  ack_token(msg.token)

                when Semtech::PullResp

                  # send now
                  if msg.txpk.imme

                    s.write(
                      Semtech::TXAck.new(
                        token: msg.token,
                        eui: @eui,
                        txpk_ack: Semtech::TXPacketAck.new(error: 'NONE')
                      ).encode
                    )

                    increment_token

                    log_debug{"transmit frame immediately (imme=true)"}

                    transmit(msg.txpk)

                  # send according to time stamp
                  elsif msg.txpk.tmst

                    timeNow = tmst

                    if msg.txpk.tmst > timeNow
                      delta = msg.txpk.tmst - timeNow
                    else
                      delta = ((2**32)-1) - timeNow + msg.txpk.tmst
                    end

                    # I expect the advance time to never be more than tens of seconds
                    if delta <= 10000000

                      log_debug{"transmit frame in #{delta}us (tmst=#{msg.txpk.tmst})"}
                      log_debug{"send up TXAck (error=NONE)"}

                      s.write(
                        Semtech::TXAck.new(
                          token: msg.token,
                          eui: @eui,
                          txpk_ack: Semtech::TXPacketAck.new(error: 'NONE')
                        ).encode
                      )

                      increment_token

                      @scenario.clock.call_in(delta) do

                        transmit(msg.txpk)

                      end

                    else

                      log_debug{"too late to transmit frame (tmst=#{msg.txpk.tmst})"}
                      log_debug{"send up TXAck (error=TOO_LATE)"}

                      s.write(
                        Semtech::TXAck.new(
                          token: msg.token,
                          eui: @eui,
                          txpk_ack: Semtech::TXPacketAck.new(error: 'TOO_LATE')
                        ).encode
                      )

                      increment_token

                    end

                  # don't support the use of time field
                  elsif msg.tkpk.time

                    log_debug{"transmit frame at #{msg.txpk.time} (not supported)"}
                    log_debug{"send up TXAck (error=GPS_UNLOCKED)"}

                    s.write(
                      Semtech::TXAck.new(
                        token: msg.token,
                        eui: @eui,
                        txpk_ack: Semtech::TXPacketAck.new(error: 'GPS_UNLOCKED')
                      ).encode
                    )

                    increment_token

                  end

                end

              rescue => e

                log_debug{"could not decode message received from network: #{e}"}
                log_debug{e.backtrace.join("\n")}

              end

            end

          end

        end

        broker.unsubscribe tx_begin
        broker.unsubscribe tx_end

        broker.publish({:station => self}, 'down')

      end

    end

    def running?
      @worker.alive?
    end

    def transmit(txpk)

      t_sym = (((2 ** txpk.sf) * ExtMAC::TICKS_PER_SECOND ) / txpk.bw)
      time = @scenario.ticks

      m1 = {
        :station => self,
        :time => time,
        :preamble => (time..(time + (8*t_sym))),
        :data => txpk.data,
        :sf => txpk.sf,
        :bw => txpk.bw,
        :cr => txpk.codr,
        :freq => (txpk.freq * 1000000).to_i,
        :power => 0,
        :gain => gain,
        :airTime => Radio.transmit_time_down(txpk.bw, txpk.sf, txpk.size),
        :location => location
      }

      m2 = {
        :station => self
      }

      broker.publish m1, "tx_begin"

      @scenario.clock.call_in(Radio.transmit_time_down(txpk.bw, txpk.sf, txpk.data.size)) do

        broker.publish m2, "tx_end"

      end

      log_debug{"transmit on sf: #{m1[:sf]} bw: #{m1[:bw]} freq: #{m1[:freq]}"}

    end

    # start worker
    def start
      if not running?
        @worker.run
      end
      self
    end

    # stop worker
    def stop
      if running?
        @q.push({:task => :stop})
        @worker.join
      end
      self
    end

    def with_mutex
      @mutex.synchronize do
        yield
      end
    end

    def add_token(token)
      if @tokens.size == MAX_TOKENS
        @tokens.pop
      end
      @tokens.unshift([token,false])
      self
    end

    def ack_token(token)
      @tokens.each do |t|
        if t[0] == token
          t[1] = true
        end
      end
      self
    end

    def tmst
      @scenario.ticks
    end

    private :with_mutex, :add_token, :ack_token, :tmst, :transmit

  end

end
