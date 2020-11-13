require 'minitest/autorun'
require 'ldl'

class TestTXAck < Minitest::Test

    include LDL

    def setup
        @state = Semtech::TXAck.new
    end

    def test_encode_default    
        
        out = @state.encode    
        
        iter = out.unpack("CS>Ca8a*").each
        
        assert_equal Semtech::Message::VERSION, iter.next
        iter.next
        
        assert_equal @state.class.type, iter.next
        
        assert_equal "\x00\x00\x00\x00\x00\x00\x00\x00", iter.next        
        
    end
    
    def test_decode_default
        
        input = @state.encode
        
        input = "\x02\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00"
        
        decoded = Semtech::PullData.decode(input)
        
        assert_equal 0,  decoded.token
        
    end
    
    def test_encode_decode
        assert_kind_of Semtech::TXAck, Semtech::Message.decode(@state.encode)        
    end

end
