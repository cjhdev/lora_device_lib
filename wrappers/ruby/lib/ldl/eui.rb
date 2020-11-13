module LDL

  class EUI

    NIBBLE = "[0-9a-fA-F]"
    OCTET = "(#{NIBBLE}#{NIBBLE})"
    DELIM1 = OCTET + "-" + OCTET + "-" + OCTET + "-" + OCTET + "-" + OCTET + "-" + OCTET + "-" + OCTET + "-" + OCTET
    DELIM2 = OCTET + ":" + OCTET + ":" + OCTET + ":" + OCTET + ":" + OCTET + ":" + OCTET + ":" + OCTET + ":" + OCTET
    DELIM3 = OCTET + " " + OCTET + " " + OCTET + " " + OCTET + " " + OCTET + " " + OCTET + " " + OCTET + " " + OCTET
    DELIM4 = OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET + OCTET

    # @param input [String] EUI64 formatted string or octet string
    def initialize(input)

      input = input.to_s

      # assume this is a binary string containing EUI64
      if input.size == 8

          @bytes = input
          @eui = ""
          input.unpack("C8").each do |octet|

              @eui << "%02X" % octet
              @eui << "-"

          end

          @eui.chop!
          @msb = @bytes.bytes.first

      # assume text string
      else

          match = Regexp.new(DELIM1).match(input)

          if match.nil?

              match = Regexp.new(DELIM2).match(input)

              if match.nil?

                  match = Regexp.new(DELIM3).match(input)

                  if match.nil?

                      match = Regexp.new(DELIM4).match(input)

                  end

              end

          end

          if match.nil?

              raise "input '#{input}' is not an EUI64"

          else

            integerArray = []
            @eui = ""

            match.captures.each do |c|

              integerArray << c.to_i(16)
              @eui << c
              @eui << "-"

            end

            @eui.chop!
            @eui = @eui.upcase

            @bytes = integerArray.pack("c*")
            @msb = @bytes.bytes.first

          end

      end

    end

    # @param input [String]
    # @return [String]
    def self.new_to_s(input)
      self.new(input).to_s
    end

    # @param input [String]
    # @return [String]
    def self.new_to_bytes(input)
      self.new(input).bytes
    end

    # @return [Integer] byte string intepreted as a big-endian number
    def to_i
      @bytes.bytes.inject(0) {|rcv,i| (rcv << 8) + i}
    end

    # @return [String] byte string
    def bytes
      @bytes
    end

    # @return [String]
    def to_s(delimiter="-")

      @eui.gsub('-', delimiter).force_encoding("ASCII-8BIT")

    end

    # @param other [EUI64]
    # @return [true] values match
    # @return [false] values do not match
    def ==(other)

      if other.is_a? EUI

        if @bytes == other.bytes

          return true

        end

      end

      false

    end

    alias eql? ==

    def hash

      @bytes.hash

    end

  end

end
