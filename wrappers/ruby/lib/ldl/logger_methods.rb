module LDL

  LOG_FORMATTER = Proc.new do |severity, datetime, progname, msg|
    "#{severity.ljust(5)} [#{datetime.strftime("%Y-%m-%d %H:%M:%S")}] #{progname}: #{msg}\n"
  end

  module LoggerMethods

    NULL_LOGGER = Logger.new(IO::NULL)
    NULL_LOGGER.level = Logger::WARN

    def _do_hdr(hdr)
      if hdr
        "#{log_header}: #{hdr}"
      else
        log_header
      end
    end

    def log_info(hdr=nil, &block)
      unless disable_log
        @scenario.logger.info(_do_hdr(hdr), &block)
      end
    end

    def log_error(hdr=nil, &block)
      unless disable_log
        @scenario.logger.debug(_do_hdr(hdr), &block)
      end
    end

    def log_debug(hdr=nil, &block)
      unless disable_log
        @scenario.logger.debug(_do_hdr(hdr), &block)
      end
    end

    def log_header
      @log_header||=self.class.name
    end

    def log_header=(hdr)
      @log_header = hdr
    end

    def disable_log
      @disable_log||=false
    end

    def disable_log=(value)
      @disable_log = value
    end

  end

end
