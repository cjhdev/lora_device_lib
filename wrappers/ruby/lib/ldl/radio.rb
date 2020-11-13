module LDL

  class Radio

    include LoggerMethods
    include RadioModel

    attr_accessor :active
    attr_reader :buffer, :gain, :location, :name, :event_cb

    def broker
      @scenario.broker
    end

    def on_event(&block)
      @event_cb = block
    end

    def initialize(scenario, **opts)

      @event_cb = nil
      @scenario = scenario
      @buffer = {}

      @name = opts[:name]||EUI.new(opts[:dev_eui]).to_s("")

      @gain = opts[:gain]||0

      @location = opts[:location]||Location.new(0,0)

      @active = []

      @logger_header = "DEVICE(#{name})::Radio"

      this = self

      broker.subscribe "tx_begin" do |m1|
        active << m1 unless m1[:station] == this
      end

      broker.subscribe "tx_end" do |m2|
        active.delete_if{|m1|m1[:station] == m2[:station]}
      end

    end

    def reset(state)
    end

    def transmit(data, **opts)

      bw = opts[:bw]
      sf = opts[:sf]
      freq = opts[:freq]

      msg = {
        station: self,
        time: @scenario.ticks,
        airTime: self.class.transmit_time_up(bw, sf, data.size),
        data: data.dup,
        sf: sf,
        bw: bw,
        cr: 1,
        freq: freq,
        power: opts[:dbm],
        gain: gain,
        location: location
      }

      broker.publish msg, "tx_begin"

      this = self

      @scenario.clock.call_in(msg[:airTime]) do

        event_cb.call(:tx_complete) if event_cb

        broker.publish({station: this}, "tx_end")

      end

      true

    end

    def receive(**opts)

      bw = opts[:bw]
      sf = opts[:sf]
      freq = opts[:freq]

      t_sym = (((2 ** opts[:sf]) * ExtMAC::TICKS_PER_SECOND ) / opts[:bw])

      log_debug{"listening on sf: #{sf} bw: #{bw} freq: #{freq} timeout: #{opts[:timeout]} symbols"}

      # work out if there were any overlapping transmissions at window timeout
      @scenario.clock.call_in( opts[:timeout] * t_sym ) do

        endtime = @scenario.ticks

        active.detect do |m1|

          # same channel, datarate, and with at least 5 preamble symbols
          m1[:sf] == sf and m1[:bw] == bw and m1[:freq] == freq and (m1[:time] < endtime) and ((endtime - m1[:time]) >= (5*t_sym))

        end.tap do |m1|

          if m1

            tx_end = broker.subscribe "tx_end" do |m2|

              if m2[:station] == m1[:station]

                log_info{"message received"}

                broker.unsubscribe tx_end
                @buffer = {
                  data: m1[:data].dup,
                  sf: m1[:sf],
                  bw: m1[:bw],
                  freq: m1[:freq],
                  rssi: -80,
                  lsnr: 9,
                }

                event_cb.call(:rx_ready) if event_cb

              end

            end

          else

            event_cb.call(:rx_timeout) if event_cb

          end

        end

      end

      true

    end

    def read_buffer
      @buffer
    end

    def set_mode
    end

  end

end
