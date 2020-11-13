module LDL::Semtech

    class PullAck < Message
    
        @type = 4
    
        def self.decode(msg)
            
            iter = msg.unpack("CS>C").each
            
            version = iter.next
            token = iter.next
            type = iter.next
            
            if version != Message::VERSION
                raise ArgumentError.new "unknown protocol version"
            end
            
            if type != self.type
                raise ArgumentError.new "expecting message type #{self.type}"
            end
            
            self.new(
                version: version,
                token: token
            )
        end
    
    end

end
