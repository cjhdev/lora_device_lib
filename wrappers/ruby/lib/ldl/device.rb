require 'forwardable'

module LDL

  class Device

    extend Forwardable

    include LoggerMethods

    attr_reader :mac, :sm, :radio

    def_delegators :@mac,
      :dev_eui,
      :join_eui,
      :otaa,
      :unconfirmed,
      :confirmed,
      :ready,
      :joined,
      :forget,
      :power,
      :power=,
      :rate,
      :rate=,
      :op,
      :state,
      :max_dcycle,
      :max_dcycle=,
      :name,
      :start,
      :stop,
      :entropy,
      :region,
      :adr=,
      :adr,
      :unlimited_duty_cycle=,
      :on_rx,
      :on_device_time,
      :on_link_status,
      :dev_addr,
      :net_id,
      :join_nonce,
      :next_dev_nonce

    def_delegators :@sm,
      :nwk_key,
      :app_key,
      :keys

    def_delegators :@radio,
      :reliability=,
      :rx_log,
      :tx_log

    def initialize(broker, clock, **opts)

      opts[:dev_eui] ||= SecureRandom.bytes(8)
      opts[:join_eui] ||= SecureRandom.bytes(8)
      opts[:nwk_key] ||= SecureRandom.bytes(16)
      opts[:app_key] ||= SecureRandom.bytes(16)

      @radio =  Radio.new(broker, clock, **opts)
      @sm = SM.new(broker, clock, **opts)
      @mac = MAC.new(broker, clock, sm, radio, **opts)

    end

  end

end
