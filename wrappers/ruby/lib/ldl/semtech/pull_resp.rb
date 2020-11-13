module LDL::Semtech

    class PullResp < Message
    
        @type = 3
        
        def self.decode(msg)
            
            iter = msg.unpack("CS>Ca*").each
            
            version = iter.next
            token = iter.next
            type = iter.next
            txpk = nil
            
            if version != Message::VERSION
                raise ArgumentError.new "unknown protocol version"
            end
            
            if type != self.type
                raise ArgumentError.new "expecting message type #{self.type}"
            end
            
            begin            
                
                root = JSON.parse(iter.next)
                
                if not root.kind_of? Hash
                    raise ArgumentError.new "expecting root JSON to be an object"
                end
                
                if not root.has_key? 'txpk'
                    raise ArgumentError.new "root must contain key 'txpk'"
                end
                
                txpk = TXPacket.from_h(root["txpk"])
                
            rescue => e
                raise ArgumentError.new "payload is not valid: #{e}"            
            end
            
            self.new(
                version: version,
                token: token,
                txpk: txpk
            )
        end
        
        attr_reader :txpk
        
        def initialize(**params)
            
            super(**params)
            
            if params.has_key? :txpk
                raise TypeError unless params[:txpk].kind_of? TXPacket
                @txpk = params[:txpk]
            else
                @txpk = TXPacket.new
            end
            
        end
        
        def encode
            [super, {:txpk => txpk}.to_json].pack("a*a*")
        end
        
    end

end
