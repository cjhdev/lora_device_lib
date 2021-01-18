module LDL

  LOG_FORMATTER = Proc.new do |severity, datetime, progname, msg|
    "#{severity.ljust(5)} [#{datetime.strftime("%Y-%m-%d %H:%M:%S")}] #{progname}#{progname ? ": " : ""}#{msg}\n"
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
      @logger.info(_do_hdr(hdr), &block) if log_is_enabled
    end

    def log_error(hdr=nil, &block)
      @logger.debug(_do_hdr(hdr), &block) if log_is_enabled
    end

    def log_debug(hdr=nil, &block)
      @logger.debug(_do_hdr(hdr), &block) if log_is_enabled
    end

    def log_header
      @log_header||=self.class.name
    end

    def log_header=(hdr)
      @log_header = hdr
    end

    def enable_log
      @log_is_enabled = true
    end

    def disable_log
      @log_is_enabled = false
    end

    def log_is_enabled
      @log_is_enabled ||= false
    end

  end

end
