#ifndef CONTROLCHANNEL_HPP_
#define CONTROLCHANNEL_HPP_

namespace barbarossa::controlchannel::v1 {

enum ControlChannelStates {
  // Initial state
  kControlChannelStatePending = 0,
  // Web socket connection is opened
  kControlChannelStateOpened = 1,
  // Hello and welcome messages exchanged successfully
  kControlChannelStateEstablished = 2,
  // Hello message is refused (with an abort message) by the server, e.g.
  // with reason ERR_NO_SUCH_RELAM
  kControlChannelStateRefused = 3,
  // Abort message received from server
  kControlChannelStateAborted = 4,
  // Heartbeat expired because we didn't received pong message within period
  kControlChannelStateExpired = 5,
  // Call or publish request isn't replied within period
  kControlChannelStateUnacknowledged = 6,
  // Control channel received an interrupt signal
  kControlChannelStateInterrupted = 7,
  // Web socket connection is closed
  kControlChannelStateClosed = 8,
  // Web socket connection is failed
  kControlChannelStateFailed = 9,
};

template <typename C>
class ControlChannel {
  ControlChannelStates state_;
  C con_;

 public:
  ControlChannel(C& con) : state_(kControlChannelStatePending), con_(con) {}
  Run() { con_.Connect(); }
};

}  // namespace barbarossa::controlchannel::v1

#endif  // CONTROLCHANNEL_HPP_
