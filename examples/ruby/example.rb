require 'ldl'
require 'securerandom'

include LDL

opts = {
  logger: Logger.new(STDOUT),
  
  dev_eui: "\x00\x00\x00\x00\x00\x00\x00\x03".force_encoding(Encoding::ASCII), 
  join_eui: SecureRandom.bytes(8),
  nwk_key: "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01".force_encoding(Encoding::ASCII),

  gw_eui: "\x00\x00\x00\x00\x00\x00\x00\x01".force_encoding(Encoding::ASCII),
  host: 'localhost',
  port: 1700
}

scenario = Scenario.new(**opts) do |device|

  device.mac.otaa

  sleep
  
end

scenario.start

begin
  sleep
rescue Interrupt
end

scenario.stop
