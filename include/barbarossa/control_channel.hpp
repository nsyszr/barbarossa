#ifndef CONTROLCHANNEL_HPP_
#define CONTROLCHANNEL_HPP_

#include <cstdint>
#include <functional>
#include <future>
#include <mutex>

#include "barbarossa/protocol.hpp"
#include "barbarossa/zmq_utils.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "zmq.hpp"
#include "zmq_addon.hpp"

using json = nlohmann::json;
namespace zmqutils = barbarossa::zmqutils;

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

std::ostream& operator<<(std::ostream& out, ControlChannelStates value) {
  switch (value) {
    case kControlChannelStatePending:
      out << "STATE_PENDING";
      break;
    case kControlChannelStateOpened:
      out << "STATE_OPENED";
      break;
    case kControlChannelStateEstablished:
      out << "STATE_ESTABLISHED";
      break;
    case kControlChannelStateRefused:
      out << "STATE_REFUSED";
      break;
    case kControlChannelStateAborted:
      out << "STATE_ABORTED";
      break;
    case kControlChannelStateExpired:
      out << "STATE_EXPIRED";
      break;
    case kControlChannelStateClosed:
      out << "STATE_CLOSED";
      break;
    case kControlChannelStateFailed:
      out << "STATE_FAILED";
      break;
    case kControlChannelStateInterrupted:
      out << "STATE_INTERRUPTED";
      break;
  }

  return out;
}

std::string AsString(ControlChannelStates value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

enum ControlChannelEvents {
  kControlChannelEventOnOpen = 0,
  kControlChannelEventOnClose = 1,
  kControlChannelEventOnFail = 2,
  // We received a message
  kControlChannelEventOnMessage = 3,
  // Main process signals interrupt (SIGINT)
  kControlChannelEventOnInterrupt = 4,
  // We did not receive a reply to hello, publish or ping
  kControlChannelEventOnTimeout = 5,
};

std::ostream& operator<<(std::ostream& out, ControlChannelEvents value) {
  switch (value) {
    case kControlChannelEventOnOpen:
      out << "EVENT_OPEN";
      break;
    case kControlChannelEventOnClose:
      out << "EVENT_CLOSE";
      break;
    case kControlChannelEventOnFail:
      out << "EVENT_FAIL";
      break;
    case kControlChannelEventOnMessage:
      out << "EVENT_MESSAGE";
      break;
    case kControlChannelEventOnInterrupt:
      out << "EVENT_INTERRUPT";
      break;
    case kControlChannelEventOnTimeout:
      out << "EVENT_TIMEOUT";
      break;
  }

  return out;
}

std::string AsString(ControlChannelEvents value) {
  std::ostringstream os;
  os << value;
  return os.str();
}


class InvalidOperationError : public std::exception {
  const char* what() const throw() { return "Invalid operation"; }
};

template <typename T>
class ControlChannel {
 public:
  ControlChannel(T& endpoint)
      : endpoint_(endpoint), state_(kControlChannelStatePending), context_(1) {
    using namespace std::placeholders;
    endpoint_.set_on_open_handler(
        std::bind(&ControlChannel::EndpointOnOpen, this));
    endpoint_.set_on_close_handler(
        std::bind(&ControlChannel::EndpointOnClose, this));
    endpoint_.set_on_fail_handler(
        std::bind(&ControlChannel::EndpointOnFail, this));
    endpoint_.set_on_message_handler(
        std::bind(&ControlChannel::EndpointOnMessage, this, _1));
  }

  void Run() {
    auto socket = zmqutils::Bind(context_, "inproc://events", ZMQ_PAIR);

    // Open the connection
    if (!endpoint_.Connect()) {
      spdlog::debug("control_channel: Failed to connect");
      // TODO(DGL) Return an error code here!
      return;
    }

    while (true) {
      zmq::multipart_t request(socket);

      // TODO(DGL) Handle exception!
      auto event = request.poptyp<ControlChannelEvents>();
      spdlog::debug("control_channel: Received an event: {}", AsString(event));

      switch (event) {
        case kControlChannelEventOnInterrupt: {
          return;
        }
        case kControlChannelEventOnOpen: {
          TransitionStateTo(kControlChannelStateOpened);
          break;
        }
        case kControlChannelEventOnClose: {
          TransitionStateTo(kControlChannelStateClosed);
          break;
        }
        case kControlChannelEventOnFail: {
          TransitionStateTo(kControlChannelStateClosed);
          break;
        }
        case kControlChannelEventOnMessage: {
          /*auto msg = request.popstr();
          HandleMessage(msg);*/
          break;
        }
        case kControlChannelEventOnTimeout: {
          /*auto msg_type = request.poptyp<protocol::MessageTypes>();
          if (msg_type == protocol::kMessageTypeHello) {
            // No respond to hello
          } else if (msg_type == protocol::kMessageTypePing) {
            // Change state
            TransitionStateTo(kControlChannelStateExpired);
          }*/
          break;
        }
      }
    }
  }

 private:
  T& endpoint_;
  ControlChannelStates state_;
  zmq::context_t context_;

  std::mutex raise_event_mutex_;

  int32_t session_id_;

  void EndpointOnOpen() { RaiseEvent(kControlChannelEventOnOpen); }
  void EndpointOnClose() { RaiseEvent(kControlChannelEventOnClose); }
  void EndpointOnFail() { RaiseEvent(kControlChannelEventOnFail); }
  void EndpointOnMessage(const std::string& payload) {
    RaiseEvent(kControlChannelEventOnMessage, payload);
  }


  void RaiseEvent(ControlChannelEvents event) {
    switch (event) {
      case kControlChannelEventOnOpen:
      case kControlChannelEventOnClose:
      case kControlChannelEventOnFail:
      case kControlChannelEventOnInterrupt: {
        RaiseEvent(event, "");
        break;
      }
      case kControlChannelEventOnMessage:
      case kControlChannelEventOnTimeout: {
        // These events need additional data!
        throw InvalidOperationError();
      }
    }
  }

  void RaiseEvent(ControlChannelEvents event, const std::string& data) {
    std::lock_guard<std::mutex> lock(raise_event_mutex_);
    auto socket = zmqutils::Connect(context_, "inproc://events", ZMQ_PAIR);

    zmq::multipart_t request;
    request.addtyp<ControlChannelEvents>(event);
    request.addstr(data);

    request.send(socket);
  }

  // Above methods are okay
  void HandleStates() {
    spdlog::debug("control_channel: Handle state: {}", AsString(state_));

    switch (state_) {
      case kControlChannelStatePending: {
        // Initial state, nothing to do!
        break;
      }
      case kControlChannelStateOpened: {
        // Send hello message to endpoint
        auto hello_msg = protocol::HelloMessage("barbarossa@test");
        json j = hello_msg;
        endpoint_.Send(j.dump());
        break;
      }
      case kControlChannelStateEstablished: {
        // Start keep alive
        break;
      }
      case kControlChannelStateRefused: {
        // Server rejected our hello message
        break;
      }
      case kControlChannelStateAborted: {
        // Received an abort message
        break;
      }
      case kControlChannelStateExpired: {
        // Keep alive expired! We didn't received a pong message within period.
        break;
      }
      case kControlChannelStateClosed: {
        // Connection is closed
        break;
      }
      case kControlChannelStateFailed: {
        // Connection is failed
        break;
      }
      case kControlChannelStateInterrupted: {
        // We received a sigint
        break;
      }
    }
  }

  void TransitionStateTo(ControlChannelStates state) {
    spdlog::warn("Change state from {} to {}.", AsString(state_),
                 AsString(state));
    state_ = state;
    HandleStates();
  }

  // void TransitionStateTo(ControlChannelStates state);
  // void HandleMessage(const std::string& payload);

  // int EstablishSession();
  // void WaitForHelloReplyOrDie();

  // void RaiseOnTimeoutEvent(protocol::MessageTypes msg_type);

  // void WaitForPongMessageOrDie();
};

}  // namespace barbarossa::controlchannel::v1

#endif  // CONTROLCHANNEL_HPP_
