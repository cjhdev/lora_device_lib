require 'json'
require 'time'

module LDL::Semtech

    class StatusPacket
    
=begin
         Name |  Type  | Function
        :----:|:------:|--------------------------------------------------------------
         time | string | UTC 'system' time of the gateway, ISO 8601 'expanded' format
         lati | number | GPS latitude of the gateway in degree (float, N is +)
         long | number | GPS latitude of the gateway in degree (float, E is +)
         alti | number | GPS altitude of the gateway in meter RX (integer)
         rxnb | number | Number of radio packets received (unsigned integer)
         rxok | number | Number of radio packets received with a valid PHY CRC
         rxfw | number | Number of radio packets forwarded (unsigned integer)
         ackr | number | Percentage of upstream datagrams that were acknowledged
         dwnb | number | Number of downlink datagrams received (unsigned integer)
         txnb | number | Number of packets emitted (unsigned integer)
=end

        def self.from_h(msg)
            begin
            
                if msg["time"]
                    time = Time.parse(msg["time"])
                else
                    time = nil
                end
            
                self.new(
                    time: time,
                    lati: msg["lati"],
                    long: msg["long"],
                    alti: msg["alti"],
                    rxnb: msg["rxnb"],
                    rxok: msg["rxok"],
                    rxfw: msg["rxfw"],
                    ackr: msg["ackr"],
                    dwnb: msg["dwnb"],
                    txnb: msg["txnb"]
                )                    
            rescue
                raise ArgumentError
            end            
        end
 
        attr_reader :time
        attr_reader :lati
        attr_reader :long
        attr_reader :alti
        attr_reader :rxnb
        attr_reader :rxok
        attr_reader :rxfw
        attr_reader :ackr
        attr_reader :dwnb
        attr_reader :txnb
    
        def initialize(**param)
            
            init = Proc.new do |iv_name, klass, default, &validation|
                
                if param[iv_name]
                    raise TypeError unless param[iv_name].kind_of? klass                                        
                    value = param[iv_name]                    
                    instance_variable_set("@#{iv_name}", value)
                else
                    instance_variable_set("@#{iv_name}", default)        
                end
                
            end
            
            init.call(:time, Time, Time.now)
            init.call(:lati, Numeric, 0)
            init.call(:long, Numeric, 0)
            init.call(:alti, Integer, 0)
            init.call(:rxnb, Integer, 0)
            init.call(:rxok, Integer, 0)
            init.call(:rxfw, Integer, 0)
            init.call(:ackr, Numeric, 0)
            init.call(:dwnb, Integer, 0)
            init.call(:txnb, Integer, 0)
            
        end
        
        def to_json(options={})
            {
                :time => @time.iso8601,
                :lati => @lati,
                :long => @long,
                :alti => @alti,
                :rxnb => @rxnb,
                :rxok => @rxok,
                :rxfw => @rxfw,
                :ackr => @ackr,
                :dwnb => @dwnb,
                :txnb => @txnb,
            }.to_json
        end
    
    end
    
end
