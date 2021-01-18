module LDL::Semtech

    class TXAck < Message

        @type = TX_ACK

        def self.decode(msg)

            iter = msg.unpack("CS>Ca8a*").each

            version = iter.next
            token = iter.next
            type = iter.next
            eui = iter.next

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

                if root["txpk_ack"].nil?
                    raise ArgumentError.new "root must contain the key 'txpk_ack'"
                end

                txpk_ack = TXPacketAck.from_h(root["txpk_ack"])

            rescue
                ArgumentError.new "payload is not valid"
            end

            self.new(
                version: version,
                token: token,
                eui: LDL::EUI.new(eui),
                txpk_ack: txpk_ack
            )
        end

        attr_reader :eui
        attr_reader :txpk_ack

        def initialize(**params)

            super(**params)

            if params.has_key? :txpk_ack
                raise TypeError unless params[:txpk_ack].kind_of? TXPacketAck
                @rxpk_ack = params[:txpk_ack]
            else
                @txpk_ack = TXPacketAck.new
            end

            if params.has_key? :eui
                @eui = LDL::EUI.new(params[:eui])
            else
                @eui = LDL::EUI.new("00-00-00-00-00-00-00-00")
            end

        end

        def encode
            [super, eui.bytes, {:txpk_ack => txpk_ack}.to_json].pack("a*a*a*")
        end

    end

end
