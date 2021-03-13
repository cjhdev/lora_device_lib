require 'json'

module LDL

  class MAC

    include LoggerMethods
    extend Forwardable

    attr_reader :ext

    def running?
      @running
    end

    def_delegators :ext,
      :dev_eui,
      :join_eui,
      :name,
      :region,
      :next_dev_nonce,
      :join_nonce,
      :net_id,
      :dev_addr,
      :session

    def initialize(broker, clock, sm, radio, **opts)

      self.enable_log

      @logger = opts[:logger]||LoggerMethods::NULL_LOGGER
      @broker = broker
      @clock = clock

      @ready = ConditionVariable.new
      @ready_mutex = Mutex.new
      @need_to_stop = false

      @mutex = Mutex.new

      @running = false

      @job_handler = nil

      @tick_ev = nil

      evstruct = Struct.new(:event, :param)

      @rx = nil
      @device_time = nil
      @link_status = nil

      @ext = ExtMAC.new(clock, sm, radio, **opts) do |ev, **params|

        case ev
        # the following go back to the request
        when :join_complete, :data_complete, :data_timeout, :op_error, :op_cancelled, :entropy, :join_exhausted

          if @job_handler
            handler = @job_handler
            @job_handler = nil
            handler.call(evstruct.new(ev, params))
          end

        when :rx

          if @rx
            @rx.call(params)
          end

        when :link_status

          if @link_status
            @link_status.call(params)
          end

        when :session_updated
        when :dev_nonce_updated
        when :device_time

          if @device_time
            @device_time.call(params)
          end

        when :channel_ready

          @ready_mutex.synchronize { @ready.signal }

        else
          log_debug{"not handling event: #{ev}"}
        end

      end

      # configure radio to callback to mac
      radio.on_event do |ev|
        ext.radio_event
        do_process()
      end

    end

    def start
      @clock.call do
        @need_to_stop = false
        do_process
        @running = true
      end
    end

    def stop
      @clock.call do
        @need_to_stop = true
        @ready_mutex.synchronize { @ready.signal }
        @clock.cancel(@tick_ev)
        @running = false
      end
    end

    def do_process

      ext.process

      ticks = ext.ticks_until_next_event

      unless ticks.nil?

        #log_debug{"call again in #{ticks} @ #{@clock.ticks}"}

        @clock.cancel(@tick_ev)

        @tick_ev = @clock.call_in(ticks) do
          do_process
        end

      end

      self

    end

    def do_set(method, *args)
      with_mutex do
        q = Queue.new
        @clock.call do
          begin
            q << ext.send(method, *args)
          rescue => e
            q << e
          end
        end
        case retval = q.pop
        when Exception
          raise retval
        else
          retval
        end
      end
    end

    def do_get(method)
      with_mutex do
        q = Queue.new
        @clock.call do
          q << ext.send(method)
        end
        q.pop
      end
    end

    def do_command(method, *args, **opts, &block)

      q = Queue.new

      @clock.call do

        begin

          q << ext.send(method, *args, **opts)

          @job_handler = block

          do_process()

        rescue => e

          q << e

        end

      end

      # get initial result
      case retval = q.pop
      when Exception
        raise retval
      end

    end

    def entropy(**opts)

      q = Queue.new

      do_command(__method__) { |ev| q << ev }

      retval = q.pop

      case retval.event
      when :entropy
        retval.param[:entropy]
      when :op_error
        raise OpError
      when :op_cancelled
        raise OpCancelled
      else
        raise "unexpected ev"
      end

    end

    def otaa(**opts)
      with_mutex do

        q = TimeoutQueue.new

        do_command(__method__) { |ev| q << ev }

        # wait for call to complete (could take forever)
        begin

          retval = q.pop(timeout: opts[:timeout])

          case retval.event
          when :join_complete
            nil
          when :join_exhausted
            raise ErrDevNonce
          when :op_cancelled
            raise OpCancelled
          else
            raise "unexpected ev"
          end

        rescue ThreadError

          sync = Queue.new

          @clock.call do

            if q.empty?
              ext.cancel
            else
              ext.forget
            end

            sync.push nil

          end

          sync.pop
          raise ThreadError.new "otaa timeout"

        end

      end

    end

    def confirmed(port=1, data, **opts)
      with_mutex do

        ready_before = opts[:timeout] ? (Time.now + opts[:timeout]) : nil

        q = Queue.new

        loop do

          begin

            do_command(__method__, port, data, **opts) { |ev| q << ev }

            retval = q.pop

            case retval.event
            when :data_complete
              break
            when :data_timeout
              raise DataTimeout
            when :op_error
              raise OpError
            when :op_cancelled
              raise OpCancelled
            else
              raise "unexpected event"
            end

          rescue ErrNoChannel

            if ready_before

              remaining = ready_before - Time.now

              raise ErrNoChannel.new "timeout waiting for channel" if remaining <= 0

              @ready_mutex.synchronize { @ready.wait(@ready_mutex, remaining) }

            else

              @ready_mutex.synchronize { @ready.wait(@ready_mutex) }

            end

          end

        end

      end

    end

    def unconfirmed(port=1, data, **opts)
      with_mutex do

        ready_before = opts[:timeout] ? (Time.now + opts[:timeout]) : nil

        q = Queue.new

        loop do

          begin

            do_command(__method__, port, data, **opts) { |ev| q << ev }

            retval = q.pop

            case retval.event
            when :data_complete
              break
            when :op_error
              raise OpError
            when :op_cancelled
              raise OpCancelled
            else
              raise "unexpected event"
            end

          rescue ErrNoChannel

            if ready_before

              remaining = ready_before - Time.now

              raise ErrNoChannel.new "timeout waiting for channel" if remaining <= 0

              @ready_mutex.synchronize { @ready.wait(@ready_mutex, remaining) }

            else

              @ready_mutex.synchronize { @ready.wait(@ready_mutex) }

            end

          end

        end

      end
    end

    def on_rx(&block)
      @rx = block
      self
    end

    def on_device_time(&block)
      @device_time = block
      self
    end

    def on_link_status(&block)
      @link_status = block
      self
    end

    def rate=(value)
      do_set(__method__, value)
    end

    def rate
      do_get(__method__)
    end

    def power=(value)
      do_set(__method__, value)
    end

    def power
      do_get(__method__)
    end

    def state
      do_get(__method__)
    end

    def op
      do_get(__method__)
    end

    def ticks_until_next_event
      do_get(__method__)
    end

    def ready
      do_get(__method__)
    end

    def joined
      do_get(__method__)
    end

    def forget
      do_get(__method__)
    end

    def adr=(value)
      do_set(__method__, value)
    end

    def adr
      do_get(__method__)
    end

    def max_dcycle=(value)
      do_set(__method__, value.to_i)
    end

    def max_dcycle
      do_get(__method__)
    end

    def unlimited_duty_cycle=(value)
      do_set(__method__, value)
    end



    def with_mutex
      @mutex.synchronize do
        yield
      end
    end

    private :with_mutex, :do_process, :do_get, :do_set

  end

end
