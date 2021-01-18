require 'securerandom'

module LDL

  class Scenario

    attr_reader :region, :gw, :device, :broker, :clock, :logger

    def self.log_info(msg)
      @logger.info{msg} if @logger
    end

    def self.log_debug(msg)
      @logger.debug{msg} if @logger
    end

    def self.log_error(msg)
      @logger.error{msg} if @logger
    end

    def self.logger=(logger)
      @logger = logger
    end

    def self.run(**opts)

      inst = self.new(**opts)
      inst.start
      yield(inst) if block_given?
      inst.stop

    end

    # @param opts [Hash]
    #
    # @option opts [Symbol] :region
    # @option opts [String] :gw_eui
    # @option opts [String] :join_eui
    # @option opts [String] :dev_eui
    # @option opts [String] :nwk_key
    # @option opts [String] :app_key
    # @option opts [String] :lorawan_version
    # @option opts [String] :budget_margin
    #
    def initialize(**opts)

      @logger = opts[:logger]||LoggerMethods::NULL_LOGGER

      self.class.logger = @logger

      @broker = Broker.new
      @clock = Clock.new(logger: logger)

      @gw = Gateway.new(broker, clock, **opts.merge(logger: logger))

      @device = Device.new(broker, clock, **opts.merge(logger: logger))

    end

    def region
      @device.region
    end

    def ticks
      @clock.ticks
    end

    def start
      clock.start
      device.start
      gw.start
      self
    end

    def stop
      gw.stop
      device.stop
      clock.stop
      self
    end

  end

end
