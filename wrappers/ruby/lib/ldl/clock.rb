
module LDL

  class Clock

    include LoggerMethods

    # @return [Integer] 32 bit system time integer
    #
    def ticks
      (@time * ExtMAC::TICKS_PER_SECOND).to_i & 0xffffffff
    end

    def ticks_to_s(ticks)
      ticks.to_f / ExtMAC::TICKS_PER_SECOND.to_f
    end

    def initialize(scenario)

      @disable_log = true

      @scenario = scenario
      @queue = []
      @running = false
      @mutex = Mutex.new
      @update = ConditionVariable.new
      @event = Struct.new(:timeout, :block, :id)
      @time = self.get_time()

      Thread.new do

        loop do

          expired = nil

          with_mutex do

            floating_time = self.get_time()

            if not(@queue.empty?) and @queue.first.timeout <= floating_time

              expired = @queue.shift

            else

              loop do

                # fixed time will now track floating
                @time = floating_time

                if @queue.empty?

                  @update.wait(@mutex)

                else

                  @update.wait(@mutex, @queue.first.timeout - floating_time)

                end

                # wakeup!

                # floating time must be updated
                floating_time = self.get_time()

                break unless @queue.empty?

              end

            end

          end

          if expired
            log_debug{"calling #{expired.id}"}
            @time = expired.timeout
            expired.block.call
          end

        end

      end.run

    end

    def on_timeout(interval, &block)

      interval = interval.to_i

      raise ArgumentError unless interval >= 0

      event = nil

      with_mutex do

        future_time = (@time + (interval.to_f / ExtMAC::TICKS_PER_SECOND.to_f)).round(6)

        event = @event.new(future_time, block, rand(0..2**32))

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

      end

      log_debug{"#{event.id} created"}

      event

    end

    def call(&block)
      call_in(0, &block)
    end

    alias_method :call_in, :on_timeout

    def cancel(event)
      if event
        with_mutex do
          if @queue.delete(event)
            log_debug{"#{event.id} deleted"}
          end
        end
      end
      self
    end

    def clear
      with_mutex do
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
      Time.now.to_f.round(6)
    end

    private :with_mutex, :get_time

  end

end
