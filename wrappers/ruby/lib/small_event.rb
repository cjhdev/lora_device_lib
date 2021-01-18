module SmallEvent

  class Broker

    def initialize
      @subscribers = {}
      @mutex = Mutex.new
    end

    # publish a message to a topic (or all topics)
    #
    # @param msg [Object]
    # @param topic [nil,String]
    # @return [self]
    #
    # @example publish to all
    #
    #   b = SmallEvent::Broker.new
    #
    #   b.publish "hello world"
    #
    # @example publish to specific topic
    #
    #   b = SmallEvent::Broker.new
    #
    #   b.publish "hello world", "specific_topic"
    #
    def publish(msg, topic=nil)

      with_mutex do
        @subscribers.keys.select do |t|
          if topic.nil?
            true
          elsif t.kind_of? Regexp
            topic[t]
          else
            t == topic
          end
        end.push(nil).map do |t|
          @subscribers[t]
        end
      end.compact.flatten.each{ |p| p.call(msg, topic) }

      self

    end

    # Subscribe to a topic
    #
    # @param topics [Array<String>] an empty array means all topics
    # @param opts [Hash]
    # @param block [Proc]
    # @return [Proc] keep this reference to unsubscribe later
    #
    # @example subscribe to all events
    #
    #   b = SmallEvent::Broker.new
    #
    #   b.subscribe do |msg, t|
    #     puts "got msg '#{msg}' for topic '#{t}'"
    #   end
    #
    # @example subscribe to one event
    #
    #   b = SmallEvent::Broker.new
    #
    #   b.subscribe("a topic") do |msg, t|
    #     puts "got msg '#{msg}' for topic '#{t}'"
    #   end
    #
    # @example subscribe to multiple events
    #
    #   b = SmallEvent::Broker.new
    #
    #   b.subscribe("a topic", "another topic") do |msg, t|
    #     puts "got msg '#{msg}' for topic '#{t}'"
    #   end
    #
    # @example subscribe to a matching event
    #
    #   b = SmallEvent::Broker.new
    #
    #   b.subscribe(/^event[0-9]+/) do |msg, t|
    #     puts "got msg '#{msg}' for topic '#{t}'"
    #   end
    #
    def subscribe(*topics, **opts, &block)

      raise ArgumentError unless block

      if topics.empty?
        topics << nil
      end

      with_mutex do
        topics.uniq.each do |t|
          if subs = @subscribers[t]
            subs << block
          else
            @subscribers[t] = [block]
          end
        end
      end

      block

    end

    # Unsubscribe from topic
    #
    # @param block [Proc] the block to unsubscribe
    # @return [self]
    #
    # @example
    #
    #   b = SmallEvent::Broker.new
    #
    #   handler = b.subscribe do
    #     puts "got event"
    #   end
    #
    #   b.unsubscribe(handler)
    #
    def unsubscribe(block)
      with_mutex do
        @subscribers.each do |k,v|
          v.delete(block)
          if v.size == 0
            @subscribers.delete(k)
          end
        end
      end
      self
    end

    # @private
    def with_mutex
      @mutex.synchronize do
        yield
      end
    end

    private :with_mutex

  end

end
