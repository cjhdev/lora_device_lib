require 'base64'
require 'json'

module LDL

  class FrameLogger

    include LoggerMethods

    def initialize(scenario)

      @scenario = scenario

      @scenario.broker.subscribe('tx_begin') do |msg|

        log_debug do
          {
            time: @scenario.clock.ticks_to_s(msg[:time]).round(5),
            station: msg[:station] ? msg[:station].name : nil,
            freq: msg[:freq],
            sf: msg[:sf],
            bw: msg[:bw]/1000,
            power: msg[:power],
            air_time: @scenario.clock.ticks_to_s(msg[:airTime]),
            message: Base64.strict_encode64(msg[:data])
          }.to_json
        end

      end

    end

  end

end
