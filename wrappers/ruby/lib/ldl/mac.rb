require 'json'

module LDL

  class MAC

    include LoggerMethods

    attr_reader :scenario, :ext

    def initialize(scenario, sm, radio, **opts)

      @scenario = scenario

      @job_events = []
      @job_handler = nil

      @tick_ev = nil

      @ext = ExtMAC.new(scenario, sm, radio, **opts) do |ev, **params|

        log_debug{"event: #{ev} #{(params.empty? ? "" : params.to_json)}"}

        if @job_handler and @job_events.include? ev
          @job_handler.call(ev)
        end

      end

      @log_header = "DEVICE(#{name})::MAC"

      # configure radio to callback to mac
      radio.on_event do |ev|
        radio_event(ev)
      end

      # ensure the stack starts ticking
      scenario.clock.call do
        do_process
      end

    end

    def do_process

      ext.process

      ticks = ext.ticks_until_next_event

      if ticks

        #log_debug{"call again in #{ticks}"}

        scenario.clock.cancel(@tick_ev)

        @tick_ev = scenario.clock.call_in(ticks) do
          do_process
        end

      end

      self

    end

    def radio_event(ev)
      ext.send(__method__, ev)
      scenario.clock.call do
        do_process()
      end
      self
    end

    def do_set(method, *args)
      q = Queue.new
      scenario.clock.call do
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

    def do_get(method)
      q = Queue.new
      scenario.clock.call do
        q << ext.send(method)
      end
      q.pop
    end

    def otaa(**opts)

      q = Queue.new

      scenario.clock.call do

        begin

          ext.otaa

          @job_handler = nil
          @job_events = [:join_complete]
          @job_handler = proc do |ev|
            q << ev
          end

          do_process()
          q << nil
        rescue => e
          q << e
        end

      end

      case retval = q.pop
      when Exception
        raise retval
      end

      q.pop

      @job_handler = nil

    end

    def confirmed(port=1, data, **opts)

      q = Queue.new

      scenario.clock.call do

        begin

          ext.confirmed(port, data, **opts)

          @job_handler = nil
          @job_events = [:join_complete]
          @job_handler = proc do |ev|
            q << ev
          end

          do_process()
          q << nil
        rescue => e
          q << e
        end

      end

      case retval = q.pop
      when Exception
        raise retval
      end

      retval = q.pop

      @job_handler = nil

      case retval
      when :data_complete
        self
      when :data_timeout
        raise DataTimeout
      when :data_nack
        raise DataNack
      when Exception
        raise retval
      else
        raise
      end

    end

    def unconfirmed(port=1, data, **opts)

      q = Queue.new

      scenario.clock.call do

        begin

          ext.unconfirmed(port, data, **opts)

          @job_handler = nil
          @job_events = [:data_complete]
          @job_handler = proc do |ev|
            q << ev
          end

          do_process()
          q << nil

        rescue => e
          q << e
        end

      end

      case retval = q.pop
      when Exception
        raise retval
      end

      q.pop

      @job_handler = nil

      raise

    end

    def on_rx(&block)

      @rx = block
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

    def seconds_since_valid_downlink
      do_get(__method__)
    end

    def dev_eui
      ext.dev_eui
    end

    def join_eui
      ext.join_eui
    end

    def name
      ext.name
    end

  end

end
