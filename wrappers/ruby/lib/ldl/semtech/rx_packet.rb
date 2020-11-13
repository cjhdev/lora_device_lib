require 'json'
require 'time'
require 'base64'

module LDL::Semtech

    class RXPacket
    
=begin
         Name |  Type  | Function
        :----:|:------:|--------------------------------------------------------------
         time | string | UTC time of pkt RX, us precision, ISO 8601 'compact' format
         tmms | number | GPS time of pkt RX, number of milliseconds since 06.Jan.1980
         tmst | number | Internal timestamp of "RX finished" event (32b unsigned)
         freq | number | RX central frequency in MHz (unsigned float, Hz precision)
         chan | number | Concentrator "IF" channel used for RX (unsigned integer)
         rfch | number | Concentrator "RF chain" used for RX (unsigned integer)
         stat | number | CRC status: 1 = OK, -1 = fail, 0 = no CRC
         modu | string | Modulation identifier "LORA" or "FSK"
         datr | string | LoRa datarate identifier (eg. SF12BW500)
         datr | number | FSK datarate (unsigned, in bits per second)
         codr | string | LoRa ECC coding rate identifier
         rssi | number | RSSI in dBm (signed integer, 1 dB precision)
         lsnr | number | Lora SNR ratio in dB (signed float, 0.1 dB precision)
         size | number | RF packet payload size in bytes (unsigned integer)
         data | string | Base64 encoded RF packet payload, padded
=end
        
        attr_reader :time
        attr_reader :tmms
        attr_reader :tmst
        attr_reader :freq
        attr_reader :chan
        attr_reader :rfch
        attr_reader :stat
        attr_reader :modu
        
        attr_reader :bw
        attr_reader :sf        
        attr_reader :datr        
        attr_reader :codr
        attr_reader :rssi
        attr_reader :lsnr
    
        def size
            @data.size
        end
    
        attr_reader :data
    
        def self.from_h(msg)
            begin
            
                if msg["data"]
                    data = Base64.decode64(msg["data"])
                else
                    data = ""
                end
                
                if msg["time"]
                    time = Time.parse(msg["time"])
                else
                    time = nil
                end
            
                if msg["stat"]
                    stat = stat_to_sym(msg["stat"])
                else
                    stat = nil
                end
            
                self.new(
                    time: time,
                    tmms: msg["tmms"],
                    tmst: msg["tmst"],
                    freq: msg["freq"],
                    chan: msg["chan"],
                    rfch: msg["rfch"],
                    stat: stat,
                    modu: msg["modu"],
                    datr: msg["datr"],
                    codr: msg["codr"],
                    rssi: msg["rssi"],
                    lsnr: msg["lsnr"],
                    size: msg["size"],
                    data: data
                )                    
            rescue => e
                raise ArgumentError.new "invalid message: #{e}"
            end            
        end
    
        def initialize(**param)

            init = Proc.new do |iv_name, klass, default|                
                if param[iv_name]
                    raise TypeError.new "expecting #{klass} for param[:#{iv_name}] but got #{param[iv_name].class}" unless klass.nil? or param[iv_name].kind_of? klass                                        
                    if block_given?
                        value = yield(param[iv_name])
                    else
                        value = param[iv_name]                    
                    end
                    instance_variable_set("@#{iv_name}", value)
                else
                    instance_variable_set("@#{iv_name}", default)        
                end                
            end
            
            init.call(:time, Time, Time.now) do |value|
                Time.parse(value)
            end
            
            init.call(:tmst, Integer, 0) 
            init.call(:freq, Numeric, 868.10)
            init.call(:chan, Integer, nil)
            init.call(:rfch, Integer, nil) 
            init.call(:stat, Symbol, :ok) do |value|
                raise RangeError unless [:ok, :fail, :nocrc].include? value
                value
            end 
            init.call(:modu, String, "LORA") do |value|
                raise RangeError unless ["LORA", "FSK"].include? value
                value            
            end
            
            if param[:datr]
                if modu == "LORA"
                    if not(match = param[:datr].match(/^SF(?<sf>[0-9]+)BW(?<bw>[0-9]+)$/))
                        raise ArgumentError.new "datr does not match format /^SF(?<sf>[0-9])BW(?<bw>[0-9]+)$/"
                    end
                    @datr = param[:datr]
                    @sf = match[:sf].to_i
                    @bw = match[:bw].to_i * 1000
                else
                    raise ArgumentError.new "datr must be numeric" unless not(param[:datr].kind_of? Numeric)
                end
            else
                if modu == "LORA"            
                    @datr = "SF7BW125"
                    @bw = 125000
                    @sf = 7
                else
                    @datar = 50000                
                end
            end
            
            init.call(:codr, String, "4/5") do |value|
                raise ArgumentError unless value[/^[0-9]+\/[0-9]+$/]
                value
            end
            
            init.call(:rssi, Integer, 0) 
            init.call(:lsnr, Integer, 0)             
            init.call(:data, String, "") 
            
            if param[:size]            
                raise TypeError unless param[:size].kind_of? Numeric
                raise ArgumentError.new("explicit size does not match size of data") unless param[:size] == data.size
            end
            
        end
        
        def to_json(options={})
            {
                :time => @time.iso8601,
                :tmst => tmst,
                :chan => chan,
                :rfch => rfch,
                :freq => freq,                
                :stat => self.class.sym_to_stat(stat),
                :modu => modu,
                :datr => datr,
                :codr => codr,
                :rssi => rssi,
                :lsnr => lsnr,
                :size => size,
                :data => Base64.strict_encode64(@data)
            }.delete_if{|k,v|v.nil?}.to_json
        end
    
        def self.sym_to_stat(s)
            case s
            when :ok
                1
            when :fail
                -1
            when :nocrc
                0
            else
                raise
            end
        end
        
        def self.stat_to_sym(s)
            case s
            when 1
                :ok
            when -1
                :fail
            when 0
                :nocrc
            else
                raise ArgumentError.new "invalid status '#{s}' must be one of [-1, 0, 1]"
            end
        end
    end
    
end
