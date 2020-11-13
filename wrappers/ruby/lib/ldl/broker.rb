module LDL

  class Broker  
  
    def initialize
      @subscribers = {}
      @mutex = Mutex.new
    end
    
    def publish(msg, topic=nil)         
    
      with_mutex do            
        [topic, nil].uniq.map do |t|
          @subscribers[t]                        
        end
      end.compact.flatten.each{ |p| p.call(msg, topic) }
      
      self
        
    end
    
    def subscribe(*topics, **opts, &block)            
        
      raise ArgumentError unless block
        
      if topics.empty?
        topics << nil
      end
        
      with_mutex do                
        topics.uniq.each do |t|                
          if subscribers = @subscribers[t]                        
            subscribers << block
          else
            @subscribers[t] = [block]
          end                    
        end            
      end            
      
      block
        
    end
    
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
    
    def with_mutex
      @mutex.synchronize do
        yield
      end
    end
    
    private :with_mutex
      
  end

end
