require 'minitest/autorun'
require 'ldl'
require 'timeout'

describe "ClockTest" do

  let(:clock){ LDL::Clock.new }
  let(:q){ Queue.new }

  before do
    Thread.abort_on_exception = true
    clock.start
  end

  after do
    clock.stop
    Thread.abort_on_exception = false
  end

  it "can wait" do
    Timeout::timeout(1){ clock.wait 500000 }
  end

  it "can return ticks" do
    clock.ticks
  end

  it "can execute on timeout" do

    clock.call_in(500000) { q.push nil }

    Timeout::timeout(1) { q.pop }

  end

  it "can execute immediately" do

    clock.call { q.push nil }

    Timeout::timeout(1) { q.pop }

  end

  it "can execute multiple timeouts in correct order" do

    clock.call do

      clock.call_in(100000) { q.push 1 }
      clock.call_in(50000) { q.push 0 }
      clock.call_in(200000) { q.push 3 }
      clock.call_in(150000) { q.push 2 }

    end

    result = []

    Timeout::timeout 1 do
      result << q.pop
      result << q.pop
      result << q.pop
      result << q.pop
    end

    iter = result.each

    assert_equal 0, iter.next
    assert_equal 1, iter.next
    assert_equal 2, iter.next
    assert_equal 3, iter.next

  end

end
