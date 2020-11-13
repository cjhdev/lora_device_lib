require 'minitest/autorun'
require 'ldl'
require 'timeout'

class TestClock < Minitest::Test

    include LDL

    def setup
      @scenario = Scenario.new
      Thread.abort_on_exception = true
    end

    def clock
      @scenario.clock
    end

    def test_wait

      Timeout::timeout 1 do
        clock.wait 500000
      end

    end

    def test_time

      clock.ticks

    end

    def test_on_timeout

        q = Queue.new

        clock.on_timeout(500000) do
            q.push nil
        end

        Timeout::timeout 1 do
            q.pop
        end

    end

    def test_zero_on_timeout

      q = Queue.new

      clock.on_timeout(0) do
        q.push nil
      end

      Timeout::timeout 1 do
        q.pop
      end

    end

    def test_zero_short_timeout

      q = Queue.new

      clock.on_timeout(42) do
        q.push nil
      end

      Timeout::timeout 1 do
        q.pop
      end

    end

    def test_multiple_on_timeout

        q = Queue.new

        clock.on_timeout 0 do

            clock.on_timeout 50000 do
              q.push 0
            end

            clock.on_timeout 100000 do
              q.push 1
            end

            clock.on_timeout 150000 do
              q.push 2
            end

            clock.on_timeout 200000 do
              q.push 3
            end

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

    def teardown
        clock.clear
        Thread.abort_on_exception = false
    end


end
