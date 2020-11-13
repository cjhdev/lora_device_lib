require 'minitest/autorun'
require 'ldl'

class TestPullAck < Minitest::Test

    include LDL

    def setup
        @state = Semtech::PullAck.new
    end

    def test_encode_default    
        
        out = @state.encode    
        
        iter = out.unpack("CS>C").each
        
        assert_equal Semtech::Message::VERSION, iter.next
        iter.next
        
        assert_equal @state.class.type, iter.next
        
    end
    
    def test_decode_default
        
        input = @state.encode
        
        input = "\x02\x00\x00\x04"
        
        decoded = Semtech::PullAck.decode(input)
        
        assert_equal 0,  decoded.token
        
    end
    
    def test_encode_decode        
        assert_kind_of Semtech::PullAck, Semtech::Message.decode(@state.encode)        
    end

end
