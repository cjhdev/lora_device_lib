require 'ldl'
require 'securerandom'

include LDL

logger = Logger.new(STDOUT)
logger.formatter = LDL::LOG_FORMATTER

opts = {
  logger: logger,
  dev_eui: [0,0,0,0,0,0,0,3].pack("C*"),
  join_eui: SecureRandom.bytes(8),
  nwk_key: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1].pack("C*"),
  gw_eui: [0,0,0,0,0,0,0,1].pack("C*"),
  host: 'localhost',
  port: 1700,
  otaa_dither: 0
}

Scenario.run(**opts) do |scenario|

  puts scenario.device.entropy

  scenario.device.otaa

  scenario.device.unconfirmed "hello world"

  scenario.device.confirmed "hello world"

end
