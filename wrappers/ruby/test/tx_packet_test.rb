require 'minitest/autorun'
require 'ldl'

class TestTXPacket < Minitest::Test

  include LDL

  def setup
    @state = Semtech::TXPacket.new
  end

  def test_to_json_default
    @state.to_json
  end

  def test_from_h
    input = {}
    Semtech::TXPacket.from_h(input)
  end

  def test_encode_decode
    Semtech::TXPacket.from_h(JSON.parse(@state.to_json))
  end

  def test_real
    input = JSON.parse('{"imme":false,"tmst":5113420,"freq":868.1,"rfch":0,"powe":14,"modu":"LORA","datr":"SF7BW125","codr":"4/5","ipol":true,"size":33,"ncrc":true,"data":"IIerSi1PqFMlnMUFB+dQbXjm0la2BtCQfCU5tKBL3eZN"}')
    Semtech::TXPacket.from_h(input)
  end

end
