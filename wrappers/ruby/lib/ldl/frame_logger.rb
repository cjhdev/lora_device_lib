require 'json'

module LDL

  class FrameLogger

    def get
      with_mutex do
        @buffer.dup
      end
    end

    def initialize(**opts)
      @buffer = []
      @limit = opts[:limit]||1000
      @mutex = Mutex.new
    end

    def log(m)
      with_mutex do
        item = {
          time: Clock.ticks_to_f(m[:time]),
          end_time: Clock.ticks_to_f(m[:time] + m[:air_time]),
          ticks: m[:ticks],
          freq: m[:freq],
          sf: m[:sf],
          bw: m[:bw],
          data: m[:data],
          air_time: Clock.ticks_to_f(m[:air_time]),
          rssi: m[:rssi],
          snr: m[:snr],
          power: m[:power]
        }
        @buffer.push(item)
        @buffer.shift if @buffer.size > @limit
        nil
      end
    end

    def with_mutex
      @mutex.synchronize do
        yield
      end
    end

    private :with_mutex

  end

end
