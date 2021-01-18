require 'socket'

module LDL

  class Gateway

    include LoggerMethods
    include RadioModel

    attr_reader :eui
    attr_reader :name

    attr_reader :host
    attr_reader :port
    attr_reader :socket

    attr_reader :keepalive

    attr_reader :broker, :clock

    attr_reader :rx_log, :tx_log

    # @param broker [Broker]
    # @param clock [Clock]
    #
    # @param opts [Hash]
    #
    # @option opts [Integer] :port
    # @option opts [String] :host
    # @option opts [Number] :keepalive
    # @option opts [String] :gw_eui
    #
    #
    #
    def initialize(broker, clock, **opts)

      # gateway logs are rather noisy
      self.enable_log

      @logger = opts[:logger]||LoggerMethods::NULL_LOGGER
      @broker = broker
      @clock = clock

      self.path_loss = opts[:path_loss] if opts[:path_loss]

      @eui = EUI.new(opts[:gw_eui]||SecureRandom.bytes(8)).bytes

      @name = EUI.new(@eui).to_s("")

      # use as message token, increment for each message sent
      @token = 0

      @port = opts[:port]||1700
      @host = opts[:host]||"localhost"
      @keepalive = opts[:keepalive]||10

      @ev_keepalive = nil
      @ev_tx_begin = nil
      @ev_tx_end = nil

      @mutex = Mutex.new
      @running = false

      @rx_log = FrameLogger.new
      @tx_log = FrameLogger.new

    end

    def transmit(txpk)

      t_sym = (((2 ** txpk.sf) * clock.tps) / txpk.bw)

      msg = {
        station: self,
        time: clock.time,
        ticks: clock.ticks,
        preamble: (tmst..(tmst + (8*t_sym))),
        data: txpk.data,
        sf: txpk.sf,
        bw: txpk.bw,
        cr: txpk.codr,
        freq: (txpk.freq * 1000000).to_i,
        power: txpk.powe.to_f,
        air_time: Radio.transmit_time_down(txpk.bw, txpk.sf, txpk.size),
        reliability: self.reliability
      }

      broker.publish(
        msg,
        "tx_begin"
      )

      tx_log.log(msg)

      clock.call_in(Radio.transmit_time_down(txpk.bw, txpk.sf, txpk.data.size)) do

        broker.publish(
          {
            station: self
          },
          "tx_end"
        )

      end

    end

    # start worker
    def start
      with_mutex do

        return if running?

        buffer = []

        @socket = UDPSocket.new
        #@socket.connect(host, port)

        @running = true

        @worker = Thread.new do

          begin

            loop do

              input, sender = @socket.recvfrom(1024, 0)

              log_debug{"recv #{input.size} bytes from #{sender}"}

              begin
                msg = Semtech::Message.decode(input)

                log_debug{"got a #{msg.class}"}

              rescue
                log_debug{"cannot decode message"}
                next
              end

              clock.call do

                case msg
                when Semtech::PullResp

                  if msg.txpk.imme

                    transmit(msg.txpk)

                    log_debug{"transmit frame immediately (imme=true)"}

                    write(
                      Semtech::TXAck.new(
                        token: msg.token,
                        eui: @eui,
                        txpk_ack: Semtech::TXPacketAck.new(error: 'NONE')
                      ).encode
                    )

                  elsif msg.txpk.tmst

                    if msg.txpk.tmst > tmst
                      delta = msg.txpk.tmst - tmst
                    else
                      delta = ((2**32)-1) - tmst + msg.txpk.tmst
                    end

                    # I expect the advance time to never be more than tens of seconds
                    if delta <= 10000000

                      log_debug{"transmit frame in #{delta}us (tmst=#{msg.txpk.tmst})"}
                      log_debug{"send up TXAck (error=NONE)"}

                      clock.call_in(delta){ transmit(msg.txpk) }

                      write(
                        Semtech::TXAck.new(
                          token: msg.token,
                          eui: @eui,
                          txpk_ack: Semtech::TXPacketAck.new(error: 'NONE')
                        ).encode
                      )

                    else

                      log_debug{"too late to transmit frame (tmst=#{msg.txpk.tmst})"}
                      log_debug{"send up TXAck (error=TOO_LATE)"}

                      write(
                        Semtech::TXAck.new(
                          token: msg.token,
                          eui: @eui,
                          txpk_ack: Semtech::TXPacketAck.new(error: 'TOO_LATE')
                        ).encode
                      )

                    end

                  # don't support the use of time field
                  elsif msg.tkpk.time

                    log_debug{"transmit frame at #{msg.txpk.time} (not supported)"}
                    log_debug{"send up TXAck (error=GPS_UNLOCKED)"}

                    write(
                      Semtech::TXAck.new(
                        token: msg.token,
                        eui: @eui,
                        txpk_ack: Semtech::TXPacketAck.new(error: 'GPS_UNLOCKED')
                      ).encode
                    )

                  end

                end

              end

            # loop do
            end

          rescue
          end

        # @worker thread
        end

        action = Proc.new do

          log_debug{"keep alive!"}

          begin
            write(
              Semtech::PullData.new(
                token: next_token,
                eui: eui
              ).encode
            )
          rescue => e
            log_debug{"#{e}"}
          end

          begin
            @ev_keepalive = clock.call_in(keepalive * clock.tps, &action)
          rescue
          end

        end

        @ev_keepalive = clock.call(&action)

        @ev_tx_begin = broker.subscribe "tx_begin" do |m1|

          buffer.push(m1) if m1[:station] != self

        end

        @ev_tx_end = broker.subscribe "tx_end" do |m2|

          if m1 = buffer.detect { |m1| m1[:station] == m2[:station] }

            if not(rf_detected?(m1[:power], m1[:bw], m1[:sf]))

              log_debug{"rf signal dropped the packet"}

            elsif rand > m1[:reliability]

              log_debug{"reliability dropped the packet"}

            else

              msg = m1.dup
              msg[:rssi] = rssi(m1[:power])
              msg[:snr] = snr(m1[:power])
              rx_log.log(msg)

              log_info{"message received: rssi=#{rssi(m1[:power])} snr=#{snr(m1[:power])}"}

              begin
                write(
                  Semtech::PushData.new(
                    token: next_token,
                    eui: eui,
                    rxpk: [
                      Semtech::RXPacket.new(
                        tmst: tmst,
                        freq: m1[:freq].to_f / 1000000.0,
                        data: m1[:data],
                        datr: "SF#{m1[:sf]}BW#{m1[:bw]/1000}",
                        rssi: rssi(m1[:power]).to_i,
                        lsnr: snr(m1[:power]).to_i,
                        rfch: 0
                      )
                    ]
                  ).encode
                )
              rescue => e
                log_debug{"#{e}"}
              end

            end

            buffer.delete(m1)

          end

        end

      end
    end

    # stop worker
    def stop
      with_mutex do

        return unless running?

        @running = false

        clock.call do

          clock.cancel(@ev_keepalive)
          broker.unsubscribe(@ev_tx_begin)
          broker.unsubscribe(@ev_tx_end)

        end

        @socket.close
        @worker.join

      end
    end

    def running?
      @running
    end

    def tmst
      clock.ticks
    end

    def next_token
      @token += 1
    end

    def with_mutex
      @mutex.synchronize do
        yield
      end
    end

    def write(msg)
      @socket.send(msg, 0, host, port)
    end

    private :tmst, :transmit, :next_token, :with_mutex, :write

  end

end
