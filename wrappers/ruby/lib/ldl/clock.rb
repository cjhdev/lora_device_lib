require 'securerandom'
require 'pry'

module LDL

  class Clock

    include LoggerMethods

    TPS = 1000000
    MAX = 0xffffffff

    # @return [Integer] 32 bit system time integer
    #
    def ticks
      (@time & MAX)
    end

    attr_reader :time

    def tps
      TPS
    end

    def self.ticks_to_f(ticks)
      ticks.to_f / TPS.to_f
    end

    def self.ticks_to_s(ticks)
      (ticks.to_f / TPS.to_f).to_s
    end

    def ticks_to_s(ticks)
      self.class.send __method__, ticks
    end

    def initialize(**opts)

      @logger = opts[:logger]||LoggerMethods::NULL_LOGGER

      #self.enable_log

      @queue = []
      @running = false
      @mutex = Mutex.new
      @update = ConditionVariable.new
      @event = Struct.new(:timeout, :block, :id)
      @time = self.get_time()
      @worker = nil
      @stack = []

    end

    def on_timeout(interval, &block)

      interval = interval.to_i

      raise ArgumentError unless interval >= 0

      event = nil

      with_mutex do

        raise unless running?
        raise if @queue.any?(nil)

        if @worker == Thread.current
          _time = @time
        else
          _time = get_time
        end

        future_time = _time + interval

        event = @event.new(future_time, block, SecureRandom.uuid)

        if @queue.empty?
          @queue.push(event)
        else
          @queue.detect{|existing|event.timeout < existing.timeout}.tap do |existing|
            if existing
              @queue.insert(@queue.index(existing), event)
            else
              @queue.push(event)
            end
          end
        end

        @update.signal

        log_debug{"created #{event.id}: #{_time} + #{interval} = #{_time + interval}"}

      end

      event

    end

    def call(&block)
      call_in(0, &block)
    end

    alias_method :call_in, :on_timeout

    def cancel(event)
      if event
        with_mutex do
          raise unless running?
          if @queue.delete(event)
            log_debug{"#{event.id} deleted"}
          end
        end
      end
      self
    end

    def clear
      with_mutex do
        raise unless running?
        @queue.clear
      end
      self
    end

    def wait(interval)
      wq = Queue.new
      on_timeout(interval) do
        wq.push nil
      end
      wq.pop
      self
    end

    def with_mutex
      @mutex.synchronize do
        yield
      end
    end

    def get_time
      t = Time.now
      ((t.to_i * TPS) + t.usec)
    end

    def running?
      @running
    end

    def start

      with_mutex do

        return if running?

        sync = Queue.new

        @worker = Thread.new do

          sync.push nil

          log_debug{"starting timer thread"}

          run = true

          while run do

            expired = nil

            with_mutex do

              floating_time = self.get_time()

              log_debug{"floating_time=#{floating_time}"}

              if @queue.empty?

                @update.wait(@mutex)

              elsif @queue.first.nil?

                log_debug{"stopping timer thread"}
                @queue.clear
                run = false

              elsif @queue.first.timeout <= floating_time

                expired = @queue.shift

              else

                @update.wait(@mutex, (@queue.first.timeout - floating_time).to_f / TPS.to_f)

              end

            # with_mutex
            end

            if expired
              log_debug{"calling #{expired.id}"}
              @time = expired.timeout
              expired.block.call
            end

          # while
          end

        # end @worker = Thread.new
        end

        sync.pop

        @running = true

      end

    end

    def stop

      return unless running?

      with_mutex do
        @queue.unshift(nil)
        @update.signal
      end

      @worker.join

      @running = false

    end

    private :with_mutex, :get_time

  end

end
