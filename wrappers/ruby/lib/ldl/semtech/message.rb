module LDL::Semtech

  class Message
  
    VERSION = 2

    @subclasses = []
    @type = nil
    
    def self.type
      @type
    end
    
    def self.inherited(subclass)
      @subclasses << subclass
    end

    def self.===(other)
      self == (other)
    end

    def self.decode(msg)
    
      iter = msg.unpack("CS>C").each
      
      version = iter.next
      iter.next
      type = iter.next
      
      if version != Message::VERSION
          raise ArgumentError.new "unknown protocol version"
      end
      
      if klass = @subclasses.detect{|m|m.type == type}
        klass.decode(msg)
      else
        raise ArgumentError.new "unknown message type"
      end
        
    end
    
    attr_reader :token
    
    def initialize(**params)
      if params[:token]
        raise TypeError unless params[:token].kind_of? Integer
        raise ArgumentError unless Range.new(0, 0xffff).include? params[:token]
        @token = params[:token]
      else
        @token = LDL::Semtech.token
      end            
    end
    
    def encode
      [VERSION, token, self.class.type].pack("CS>C")
    end
  
  end

end
