require 'minitest/autorun'
require 'ldl'

class TestPullData < Minitest::Test

    include LDL

    def setup
        @state = Semtech::PullData.new
    end

    def test_encode_default    
        
        out = @state.encode    
        
        iter = out.unpack("CS>Ca8").each
        
        assert_equal Semtech::Message::VERSION, iter.next
        iter.next
        
        assert_equal @state.class.type, iter.next
        
        assert_equal "\x00\x00\x00\x00\x00\x00\x00\x00", iter.next        
        
    end
    
    def test_decode_default
        
        input = "\x02\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00"
        
        decoded = Semtech::PullData.decode(input)
        
        assert_equal 0,  decoded.token
        
    end
    
    def test_encode_decode        
        assert_kind_of Semtech::PullData, Semtech::Message.decode(@state.encode)        
    end

end
