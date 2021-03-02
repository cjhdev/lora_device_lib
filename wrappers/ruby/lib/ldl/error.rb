module LDL

  # used to indicate the state is not right (e.g. you cannot personalise a joined device)
  class StateError < StandardError
  end

  # timeout condition while waiting for join confirmation
  class JoinTimeout < StandardError
  end

  #  timeout condition while waiting for confirmed send confirmation
  class DataTimeout < StandardError
  end

  # requested operation failed for radio error
  class OpError < StandardError
  end

  # requested operation was cancelled
  class OpCancelled < StandardError
  end

  # generic run-time error...
  class Error < StandardError
  end

  class Errno < StandardError; end

  class ErrNoChannel < Errno; end
  class ErrSize < Errno; end
  class ErrRate < Errno; end
  class ErrPort < Errno; end
  class ErrBusy < Errno; end
  class ErrNotJoined < Errno; end
  class ErrPower < Errno; end
  class ErrMACPriority < Errno; end
  class ErrDevNonce < Errno; end

end
