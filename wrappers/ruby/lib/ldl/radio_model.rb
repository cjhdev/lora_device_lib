module LDL

  module RadioModel
  
    include Math

=begin
    def snr_and_rssi(loc_a, loc_b, freq, bw, sf, tx_power)
      distance = distance_between(loc_a, loc_b)
      loss = free_path_loss(distance, freq)        
      lb = link_budget(tx_power, bw, sf)
      rssi
    end
=end    

    def free_path_loss(distance, freq)        
      20*log10(distance) + 20*log10(freq) - 147.55    
    end
    
    # good
    def distance_between(a, b)    
      sqrt((b.x - a.x)**2 + (b.y - a.y)**2)
    end
    
    SNR_LIMIT = {
      7 => -7.5,
      8 => -10.0,
      9 => -12.5,
      10 => -15.0,      
      11 => -17.5,      
      12 => -20.0,      
    }
    
    NOISE_FIGURE = 6.0
    
    NOISE_POWER = -122.9
    
    # good
    def sensitivity(bw, sf)
      -174.0 + 10.0*log10(bw.to_f) + NOISE_FIGURE + SNR_LIMIT[sf]
    end
    
    # good
    def link_budget(tx_power, bw, sf)      
      tx_power - sensitivity(bw, sf)      
    end
    
    def snr(rx_power, bw, sf)
      sensitivity(bw, sf) - SNR_LIMIT[sf]
    end
    
  end

end
