module LDL

  class Scenario

    attr_reader :region, :gw, :device, :broker, :clock, :logger

    def self.log_info(msg)
      @logger.info(msg) if @logger
    end

    def self.log_debug(msg)
      @logger.debug(msg) if @logger
    end

    def self.log_error(msg)
      @logger.error(msg) if @logger
    end

    def self.logger=(logger)
      @logger = logger
    end

    # @param opts [Hash]
    #
    # @option opts [String] :gw_eui
    # @option opts [String] :join_eui
    # @option opts [String] :dev_eui
    # @option opts [String] :dev_key
    # @option opts [String] :app_key
    # @option opts [String] :lorawan_version
    # @option opts [String] :budget_margin
    #
    # @param app [Block] this is the device application
    #
    def initialize(**opts, &app)

      @logger = opts[:logger]||LoggerMethods::NULL_LOGGER

      self.class.logger = @logger

      unless opts[:dev_eui]
        opts[:dev_eui] = SecureRandom.bytes(8)
      end

      unless opts[:join_eui]
        opts[:join_eui] = SecureRandom.bytes(8)
      end

      @broker = Broker.new
      @clock = Clock.new(self)
      @frame_logger = FrameLogger.new(self)

      @gw = Gateway.new(self, **opts)
      @device = Device.new(self, **opts, &app)

    end

    def ticks
      @clock.ticks
    end

    def start
      gw.start
      device.start
      self
    end

    def stop
      gw.stop
      device.stop
      self
    end

  end

end
