require 'minitest/autorun'
require 'ldl'

class TestPushData < Minitest::Test

    include LDL

    def setup
        @state = Semtech::PushData.new
    end

    def test_encode_default

        out = @state.encode

        iter = out.unpack("CS>Ca8a*").each

        assert_equal Semtech::Message::VERSION, iter.next
        iter.next

        assert_equal @state.class.type, iter.next

        assert_equal "\x00\x00\x00\x00\x00\x00\x00\x00", iter.next

        assert_equal({"rxpk" => []}, JSON.parse(iter.next))

    end

    def test_decode_default

        input = "\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00{\"rxpk\":[]}"

        decoded = Semtech::PushData.decode(input)

        assert_equal 0,  decoded.token
        assert_equal EUI.new("00-00-00-00-00-00-00-00"), decoded.eui
        assert_nil decoded.stat

    end

    def test_encode_decode
        assert_kind_of Semtech::PushData, Semtech::Message.decode(@state.encode)
    end

end
