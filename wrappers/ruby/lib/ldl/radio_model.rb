module LDL

  module RadioModel

    include Math

    SNR_LIMIT = {
      7 => -7.5,
      8 => -10.0,
      9 => -12.5,
      10 => -15.0,
      11 => -17.5,
      12 => -20.0,
    }

    attr_writer :noise_figure

    def noise_figure
      @noise_figure ||= 6.0
    end

    attr_writer :path_loss

    def path_loss
      @path_loss ||= 50.0
    end

    attr_writer :noise_floor

    def noise_floor
      @noise_floor ||= -120.0
    end

    def sensitivity(bw, sf)
      -174.0 + 10.0*log10(bw.to_f) + self.noise_figure + SNR_LIMIT[sf]
    end

    def link_budget(tx_power, bw, sf)
      tx_power - self.sensitivity(bw, sf)
    end

    def snr(tx_power)
      self.rssi(tx_power) - self.noise_floor
    end

    def rssi(tx_power)
      tx_power - self.path_loss
    end

    def rf_detected?(tx_power, bw, sf)
      (self.link_budget(tx_power, bw, sf) > self.path_loss)
    end

    def reliability=(setting)
      @reliability = setting
    end

    def reliability
      if @reliability.nil?
        1.0
      else
        if @reliability.kind_of? Array
          setting = @reliability.shift||1.0
        else
          setting = @reliability
        end
        if setting.kind_of? Range
          rand(setting)
        elsif setting.nil?
          1.0
        else
          setting
        end
      end
    end

  end

end
