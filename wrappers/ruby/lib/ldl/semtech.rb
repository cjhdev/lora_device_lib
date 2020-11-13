module LDL

  # This module contains an implementation of the Semtech packet 
  # forwarder protocol.
  #
  module Semtech
  
    @token = rand(0..0xffff)

    # @return a message "token"
    def self.token
      result = @token % 0xffff
      @token = @token.next
      result        
    end
  
  end
      
end

require_relative 'semtech/message'
require_relative 'semtech/push_data'
require_relative 'semtech/push_ack'
require_relative 'semtech/pull_data'
require_relative 'semtech/pull_ack'
require_relative 'semtech/pull_resp'
require_relative 'semtech/tx_ack'

require_relative 'semtech/status_packet'
require_relative 'semtech/rx_packet'
require_relative 'semtech/tx_packet'
require_relative 'semtech/tx_packet_ack'
