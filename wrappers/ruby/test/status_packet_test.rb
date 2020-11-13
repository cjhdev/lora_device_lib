require 'minitest/autorun'
require 'ldl'

class TestStatusPacket < Minitest::Test

    include LDL

    def setup
        @state = Semtech::StatusPacket.new
    end

    def test_to_json_default
        @state.to_json
    end

    def test_from_h
        input = {}        
        Semtech::StatusPacket.from_h(input)
    end
    
end
