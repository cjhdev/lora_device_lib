module LDL

  class Device

    include LoggerMethods

    attr_reader :mac, :sm, :radio, :scenario, :name

    def broker
      scenario.broker
    end

    def initialize(scenario, **opts)

      @running = false
      @scenario = scenario

      @radio =  Radio.new(scenario, **opts)
      @sm = SM.new(scenario, **opts)
      @mac = MAC.new(scenario, sm, radio, **opts)

      @name = opts[:name]||mac.dev_eui.to_s("")

      @log_header = "DEVICE(#{name})"

      @worker = Thread.new do

        broker.publish({:eui => dev_eui}, "up")

        begin

          # wait for stack to become ready
          loop do
            break if @mac.ready
            @scenario.clock.wait 1000000
          end

          yield(self) if block_given?

        rescue Interrupt
        rescue => e
          log_error e
          raise
        end

        broker.publish({:eui => dev_eui}, "down")

      end

    end

    def dev_eui
      mac.send __method__
    end

    def join_eui
      mac.send __method__
    end

    def running?
      @running
    end

    def start
      if not running?
        @worker.run
        @running = true
      end
      self
    end

    def stop
      if running?
        @worker.raise Interrupt
        @worker.join
        @running = false
      end
      self
    end

  end

end
