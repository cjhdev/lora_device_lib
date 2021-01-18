module LDL

  class Radio

    include LoggerMethods
    include RadioModel

    attr_accessor :active
    attr_reader :buffer, :name, :event_cb

    attr_reader :broker, :clock, :rx_log, :tx_log

    def on_event(&block)
      @event_cb = block
    end

    def initialize(broker, clock, **opts)

      self.enable_log

      @logger = opts[:logger]||LoggerMethods::NULL_LOGGER
      @broker = broker
      @clock = clock

      @rx_log = FrameLogger.new
      @tx_log = FrameLogger.new

      self.path_loss = opts[:path_loss] if opts[:path_loss]

      @event_cb = nil
      @buffer = {}

      @name = EUI.new(opts[:dev_eui]).to_s("")

      @active = []

      @logger_header = "DEVICE(#{name})::Radio"

      @tx = false
      @rx = false
      @to = false

      this = self

      broker.subscribe "tx_begin" do |m1|
        active << m1 unless m1[:station] == this
      end

      broker.subscribe "tx_end" do |m2|
        active.delete_if{|m1|m1[:station] == m2[:station]}
      end

    end

    def transmit(data, **opts)

      bw = opts[:bw]
      sf = opts[:sf]
      freq = opts[:freq]

      msg = {
        station: self,
        time: clock.time,
        ticks: clock.ticks,
        air_time: self.class.transmit_time_up(bw, sf, data.size),
        data: data.dup,
        sf: sf,
        bw: bw,
        cr: 1,
        freq: freq,
        power: opts[:dbm],
        reliability: self.reliability
      }

      tx_log.log(msg)

      broker.publish msg, "tx_begin"

      this = self

      clock.call_in(msg[:air_time]) do

        @timeout = false
        @tx = true
        @rx = false

        event_cb.call(:tx_complete) if event_cb

        broker.publish({station: this}, "tx_end")

      end

      true

    end

    def do_timeout
      @timeout = true
      @tx = false
      @rx = false
      event_cb.call(:rx_timeout) if event_cb
    end

    def receive(**opts)

      bw = opts[:bw]
      sf = opts[:sf]
      freq = opts[:freq]

      t_sym = (((2 ** opts[:sf]) * ExtMAC::TICKS_PER_SECOND ) / opts[:bw])

      log_debug{"listening on sf: #{sf} bw: #{bw} freq: #{freq} timeout: #{opts[:timeout]} symbols"}

      # work out if there were any overlapping transmissions at window timeout
      clock.call_in( opts[:timeout] * t_sym ) do

        endtime = clock.time

        m1 = active.detect do |m|

          # same channel, datarate, and with at least 5 preamble symbols
          m[:sf] == sf and m[:bw] == bw and m[:freq] == freq and (m[:time] < endtime) and ((endtime - m[:time]) >= (5*t_sym))

        end

        if m1.nil?

          do_timeout

        elsif not(rf_detected?(m1[:power], m1[:bw], m1[:sf]))

          log_debug{"rf signal dropped the packet"}
          do_timeout

        elsif rand > m1[:reliability]

          log_debug{"reliability dropped the packet"}
          do_timeout

        else

          tx_end = broker.subscribe "tx_end" do |m2|

            if m2[:station] == m1[:station]

              msg = m1.dup
              msg[:rssi] = rssi(m1[:power])
              msg[:snr] = snr(m1[:power])
              rx_log.log(msg)

              log_info{"message received: rssi=#{rssi(m1[:power])} snr=#{snr(m1[:power])}"}

              broker.unsubscribe tx_end
              @buffer = {
                data: m1[:data].dup,
                sf: m1[:sf],
                bw: m1[:bw],
                freq: m1[:freq],
                rssi: rssi(m1[:power]),
                lsnr: snr(m1[:power])
              }

              @timeout = false
              @tx = false
              @rx = true

              event_cb.call(:rx_ready) if event_cb

            end

          end

        end

      end

      true

    end

    def read_buffer
      @buffer
    end

    def set_mode(mode)
      @tx = false
      @rx = false
      @timeout = false
    end

    def get_status

      log_debug{"get_status: tx=#{@tx} rx=#{@rx} timeout=#{@timeout}"}

      {
        tx: @tx,
        rx: @rx,
        timeout: @timeout
      }

    end

  end

end
