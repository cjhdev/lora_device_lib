require 'json'

module LDL::Semtech

    class TXPacketAck
    
=begin
         Name |  Type  | Function
        :----:|:------:|------------------------------------------------------------------------------
        error | string | Indication about success or type of failure that occured for downlink request.

        The possible values of "error" field are:

         Value             | Definition
        :-----------------:|---------------------------------------------------------------------
         NONE              | Packet has been programmed for downlink
         TOO_LATE          | Rejected because it was already too late to program this packet for downlink
         TOO_EARLY         | Rejected because downlink packet timestamp is too much in advance
         COLLISION_PACKET  | Rejected because there was already a packet programmed in requested timeframe
         COLLISION_BEACON  | Rejected because there was already a beacon planned in requested timeframe
         TX_FREQ           | Rejected because requested frequency is not supported by TX RF chain
         TX_POWER          | Rejected because requested power is not supported by gateway
         GPS_UNLOCKED | Rejected because GPS is unlocked, so GPS timestamp cannot be used
=end

        ERROR = [
            "NONE",
            "TOO_LATE",
            "COLLISION_PACKET",
            "COLLISION_BEACON",
            "TX_FREQ",
            "TX_POWER",
            "GPS_UNLOCKED"
        ]

        def self.from_h(msg)            
            begin                            
                self.new(error: msg["error"])                
            rescue
                raise ArgumentError
            end
        end
    
        attr_reader :error
        
        def initialize(**param)
            if param[:error]
                raise TypeError unless param[:error].kind_of? String
                raise RangeError unless ERROR.include? param[:error]
                @error = param[:error]
            else
                @error = ERROR.first
            end
        end
        
        def to_json(options={})
            {
                :error => @error
            }.to_json
        end
    
    end
    
end
