class TimeoutQueue
    
  # @param opts [Hash]
  #
  # @option opts [Integer] :max limit size of queue to max elements (silent drop)
  #
  def initialize(**opts)
  
    @queue = []
    @mutex = Mutex.new
    @received = ConditionVariable.new
    @closed = false
    @waiting = []
    @max = opts[:max]
    
  end
  
  # push object into end of queue
  #
  # @param object
  # @return object
  #
  def push(object, **opt)
    __push(object) do 
      @queue.send(__method__, object)
    end  
  end
  
  # push object into front of the queue
  #
  # @param object
  # @return object
  #
  def unshift(object)
    __push(object) do 
      @queue.send(__method__, object)
    end
  end
  
  # delete an object from the queue before it can be popped
  #
  # @param object
  # @return object
  #
  def delete(object)
  
    with_mutex do        
      @queue.delete(object)                                
    end
  
    object
  
  end
  
  # retrieve next object from queue
  #
  # @param non_block [TrueClass,FalseClass] set true to enable non-blocking mode
  # @param opts [Hash]
  #
  # @option opts [Float,Integer] :timeout seconds to wait (in blocking mode)
  #
  # @raise ThreadError if timeout expires or queue is empty in non_block mode
  # 
  def pop(non_block=false, **opts)
  
    timeout = opts[:timeout]
  
    with_mutex do

      @waiting << Thread.current
    
      if timeout              
        end_time = Time.now + timeout.to_f    
      end
    
      while @queue.empty? and not(non_block) and not(@closed)
      
        if timeout
        
          break unless ((time_now = Time.now) < end_time)            
          
          @received.wait(@mutex, end_time - time_now)
            
        else
        
          @received.wait(@mutex)
            
        end
      
      end 
      
      @waiting.delete(Thread.current)
      
      if @queue.empty?
      
        if @closed
          raise ClosedQueueError          
        else
          raise ThreadError
        end
        
      else
                             
        @queue.shift
      
      end
            
    end
  
  end
  
  def num_waiting
    with_mutex do
      @waiting.size
    end
  end
  
  def closed?
    @closed
  end

  def close
    with_mutex do
      if not @closed
        @closed = true
        @waiting.each(&:wakeup)
      end
    end
    self
  end

  def empty?
    @queue.send __method__
  end

  def size
    @queue.send __method__
  end
  
  def clear
    with_mutex do
      @queue.clear
    end
    self
  end

  def with_mutex
    @mutex.synchronize do
      yield
    end
  end
  
  def __push(object)
    with_mutex do   
      raise ClosedQueueError if closed?      
      return if @max and @queue.size == @max 
      yield
      @received.signal                
    end    
    object
  end

  alias_method :length, :size
  
  alias_method :'<<', :push
  alias_method :enq, :push

  alias_method :shift, :pop
  alias_method :deq, :pop
  
  private :with_mutex, :__push

end
