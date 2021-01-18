require 'ldl'
require 'securerandom'

include LDL

logger = Logger.new(STDOUT)
logger.formatter = LDL::LOG_FORMATTER

opts = {
  logger: logger,
  dev_eui: "\x00\x00\x00\x00\x00\x00\x00\x03".force_encoding(Encoding::ASCII),
  join_eui: SecureRandom.bytes(8),
  nwk_key: "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01".force_encoding(Encoding::ASCII),
  gw_eui: "\x00\x00\x00\x00\x00\x00\x00\x01".force_encoding(Encoding::ASCII),
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
