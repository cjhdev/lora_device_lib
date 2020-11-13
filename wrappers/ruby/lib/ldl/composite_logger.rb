require 'logger'

module LDL

  class CompositeLogger
    
    def initialize
      @loggers = []
    end
    
    def <<(logger)
      @loggers << logger
      self
    end

    def error(*args, &block)
      @loggers.each do |logger|
        logger.error(*args, &block)
      end
      self
    end
    
    def info(*args, &block)
      @loggers.each do |logger|
        logger.info(*args, &block)
      end
      self
    end
    
    def debug(*args, &block)
      @loggers.each do |logger|
        logger.debug(*args, &block)
      end
      self
    end
  
  end

end
