require 'json'

module LDL::Semtech

    class PushData < Message

        @type = PUSH_DATA

        def self.decode(msg)

            iter = msg.unpack("CS>Ca8a*").each

            version = iter.next
            token = iter.next
            type = iter.next
            eui = iter.next
            rxpk = nil
            stat = nil

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

                if root["rxpk"].nil? and root["stat"].nil?
                    raise ArgumentError.new "root must contain key(s) 'rxpk' or 'stat'"
                end

                if root.has_key? "rxpk"
                    rxpk = root["rxpk"].map{ |p| RXPacket.from_h(p) }
                else
                    rxpk = []
                end

                if root.has_key? "stat"
                    stat = StatusPacket.from_h(root["stat"])
                end

            rescue => e
                raise ArgumentError.new "payload is not valid: #{e}"
            end

            self.new(
                version: version,
                token: token,
                eui: LDL::EUI.new(eui),
                rxpk: rxpk,
                stat: stat
            )

        end

        attr_reader :eui
        attr_reader :rxpk
        attr_reader :stat

        def initialize(**params)

            super(**params)

            if params[:rxpk]
                raise TypeError.new "expecting params[:rxpk] to be kind of Array" unless params[:rxpk].kind_of? Array
                raise TypeError unless params[:rxpk].select{|p| not p.kind_of? RXPacket}.empty?
                @rxpk = params[:rxpk]
            else
                @rxpk = []
            end

            if params[:stat]
                raise TypeError unless params[:stat].kind_of? StatusPacket
                @stat = params[:stat]
            end

            if params[:eui]
                @eui = LDL::EUI.new(params[:eui])
            else
                @eui = LDL::EUI.new("00-00-00-00-00-00-00-00")
            end

        end

        def encode

            obj = {
                :rxpk => rxpk,
                :stat => stat
            }.delete_if { |k,v|v.nil? }

            [super, eui.bytes, obj.to_json].pack("a*a*a*")

        end

    end

end
