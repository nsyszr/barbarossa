#ifndef CONTROLCHANNEL_HPP_
#define CONTROLCHANNEL_HPP_

#include <cstdint>

#include "zmq.hpp"

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
  // Keep alive expired because we didn't received pong message within period
  kControlChannelStateExpired = 5,
  // Web socket connection is closed
  kControlChannelStateClosed = 6,
  // Web socket connection is failed
  kControlChannelStateFailed = 7,
  // Control channel received an interrupt signal
  kControlChannelStateInterrupted = 8,
};

enum ControlChannelEvents {
  kControlChannelEventOnOpen = 0,
  kControlChannelEventOnClose = 1,
  kControlChannelEventOnFailed = 2,
  // We received a message
  kControlChannelEventOnMessage = 3,
  // Main process signals interrupt (SIGINT)
  kControlChannelEventOnInterrupt = 4,
  // We did not receive a reply to hello, publish or ping
  kControlChannelEventOnTimeout = 5,
};

class Connection {
 public:
  void Connect();
  int SendMessage(const std::string_view& msg);
};

class ControlChannel {
  ControlChannelStates state_;
  Connection con_;
  // Req/Rep socket for communication with connection and application (sigint)
  zmq::context_t context_;
  zmq::socket_t socket_;

  int32_t session_id_;

  void HandleStates();
  void TransitionStateTo(ControlChannelStates state);
  void HandleMessage(const std::string_view& msg);

  int EstablishSession();
  void WaitForWelcomeMessageOrDie();
  void WaitForPongMessageOrDie();

 public:
  ControlChannel(Connection& con)
      : state_(kControlChannelStatePending),
        con_(con),
        context_(1),
        socket_(context_, ZMQ_REP) {}
  void Run();
};

}  // namespace barbarossa::controlchannel::v1

#endif  // CONTROLCHANNEL_HPP_
