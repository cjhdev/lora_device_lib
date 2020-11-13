module LDL

  class Key
  
    attr_reader :value

    def bytes
      value
    end

    NIBBLE = "[0-9a-fA-F]"
    OCTET = "(#{NIBBLE}#{NIBBLE})"
    DELIM4 = OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET

    def initialize(value)
      raise TypeError unless value.kind_of? String            
      if value.size == 16
        @value = value
      elsif match = Regexp.new(DELIM4).match(value)
        integerArray = []
        match.captures.each do |c|
          integerArray << c.to_i(16)                    
        end
        @value = integerArray.pack("c*")
      else
        raise ArgumentError
      end
            
    end
  
  end

end
