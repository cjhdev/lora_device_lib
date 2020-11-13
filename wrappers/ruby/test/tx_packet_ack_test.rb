require 'minitest/autorun'
require 'ldl'

class TestTXPacketAck < Minitest::Test

  include LDL

  def setup
    @state = Semtech::TXPacketAck.new
  end

  def test_to_json_default
    @state.to_json
  end

  def test_from_h
    Semtech::TXPacketAck.from_h({})
  end
  
  def test_encode_decode
    Semtech::TXPacketAck.from_h(JSON.parse(@state.to_json))        
  end
    
end
