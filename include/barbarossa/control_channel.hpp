#ifndef CONTROLCHANNEL_HPP_
#define CONTROLCHANNEL_HPP_

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>

#include "barbarossa/protocol.hpp"
#include "barbarossa/protocol_details.hpp"
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
          NextState(kControlChannelStateOpened);
          break;
        }
        case kControlChannelEventOnClose: {
          NextState(kControlChannelStateClosed);
          break;
        }
        case kControlChannelEventOnFail: {
          NextState(kControlChannelStateClosed);
          break;
        }
        case kControlChannelEventOnMessage: {
          auto payload = request.popstr();
          NextState(HandleEventOnMessage(payload));
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

  // acccesors
  // mutators
  void set_session_id(int32_t session_id) { session_id_ = session_id; }
  void set_session_timeout(int session_timeout) {
    session_timeout_ = session_timeout;
  }
  void set_ping_interval(int ping_interval) { ping_interval_ = ping_interval; }
  void set_pong_timeout(int pong_timeout) { pong_timeout_ = pong_timeout; }
  void set_events_topic(const std::string events_topic) {
    events_topic_ = events_topic;
  }

 private:
  // Keep the order of ctor initialization
  T& endpoint_;
  ControlChannelStates state_;
  zmq::context_t context_;

  // Other private member variables
  std::mutex state_mutex_;
  std::mutex raise_event_mutex_;

  std::condition_variable hello_message_reply_;
  std::mutex hello_message_reply_mutex_;

  std::condition_variable heartbeat_;
  std::mutex heartbeat_mutex_;


  int32_t session_id_;
  int session_timeout_;
  int ping_interval_;
  int pong_timeout_;
  std::string events_topic_;

  void EndpointOnOpen() { RaiseEvent(kControlChannelEventOnOpen); }
  void EndpointOnClose() { RaiseEvent(kControlChannelEventOnClose); }
  void EndpointOnFail() { RaiseEvent(kControlChannelEventOnFail); }
  void EndpointOnMessage(const std::string& payload) {
    spdlog::debug("control_channel: Message payload: '{}'", payload);
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

  ControlChannelStates HandleEventOnMessage(const std::string& payload) {
    auto msg = protocol::Parse(payload);
    if (msg.GetMessageType() == protocol::kMessageTypeWelcome) {
      spdlog::debug("control_channel: Release hello message reply condition");
      std::unique_lock<std::mutex> lock(hello_message_reply_mutex_);
      hello_message_reply_.notify_one();

      // Save the settings of the welcome message
      auto welcome_msg = msg.Get<protocol::welcomemessage::WelcomeMessage>();
      set_session_id(welcome_msg.session_id());

      auto welcome_msg_details =
          welcome_msg.details<protocol::welcomedetails::WelcomeDetails>();
      set_session_timeout(welcome_msg_details.session_timeout());
      set_ping_interval(welcome_msg_details.ping_interval());
      set_pong_timeout(welcome_msg_details.pong_timeout());
      set_events_topic(welcome_msg_details.events_topic());

      SetupHeartbeat();

      return kControlChannelStateEstablished;
    } else if (msg.GetMessageType() == protocol::kMessageTypeAbort) {
      ControlChannelStates state;
      // Release mutex inside scope
      {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state = state_;
      }

      if (state == kControlChannelStateOpened) {
        return kControlChannelStateRefused;
      } else {
        return kControlChannelStateAborted;
      }
    } else if (msg.GetMessageType() == protocol::kMessageTypePong) {
      auto socket = zmqutils::Connect(context_, "inproc://pong", ZMQ_PAIR);
      zmqutils::SendString(socket, "PONG");
      return kControlChannelStateEstablished;
    }
  }

  void SetupHeartbeat() {
    std::thread([&]() {
      std::chrono::seconds ping_interval(ping_interval_);
      std::chrono::seconds pong_timeout(pong_timeout_);

      while (true) {
        std::unique_lock<std::mutex> lock(heartbeat_mutex_);

        spdlog::debug("Wait until sending ping or die.");
        // We send every given seconds a ping or we stop the hearbeat
        if (heartbeat_.wait_for(lock, ping_interval) ==
            std::cv_status::no_timeout) {
          spdlog::info(
              "contorl_channel: Terminate heartbeat. Received the stop "
              "condition.");
          break;  // Exit the loop because stop_heartbeat condition is set
        }

        // Send ping
        spdlog::debug("contorl_channel: Sending ping.");
        /*auto ping_socket =
            zmqutils::Connect(context_, "inproc://ping", ZMQ_PAIR);
        zmqutils::SendString(ping_socket, "PING"); */
        auto ping_msg = protocol::PingMessage();
        json j = ping_msg;
        endpoint_.Send(j.dump());

        // Create a async wait ping reply routine which returns a future
        auto request = std::async(
            [](zmq::context_t& context, std::chrono::seconds pong_timeout) {
              // auto context = zmq::context_t(1);
              auto socket = zmqutils::Bind(context, "inproc://pong", ZMQ_PAIR);

              zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};

              zmq::poll(&items[0], 1, pong_timeout);

              if (items[0].revents & ZMQ_POLLIN) {
                auto msg = zmqutils::RecvString(socket);
                spdlog::info("Got message: {}", msg);
                return true;
              }

              return false;
            },
            std::ref(context_), std::chrono::seconds(pong_timeout_));

        // Wait until we get a reply
        spdlog::debug("contorl_channel: Waiting for pong.");
        bool reply = request.get();
        spdlog::debug("contorl_channel: Pong result: {}");

        if (reply == false) {
          spdlog::warn(
              "contorl_channel: Terminate heartbeat. We didn't received a "
              "pong.");
          break;  // Exit the loop because ping wasn't replied within time.
        }
      }
    })
        .detach();
  }

  // Above methods are okay
  void NextState(ControlChannelStates state) {
    spdlog::debug("control_channel: Handle state from {} to {}.",
                  AsString(state_), AsString(state));

    ControlChannelStates current_state;

    // To release the mutex we change the state inside a scope
    {
      std::lock_guard<std::mutex> lock(state_mutex_);

      if (state == state_) {
        // State didn't changed, nothing todo
        spdlog::debug("control_channel: Warning! State didn't changed");
        return;
      }

      state_ = state;
      current_state = state;
    }

    switch (current_state) {
      case kControlChannelStatePending: {
        // Initial state, nothing to do!
        break;
      }
      case kControlChannelStateOpened: {
        // Send hello message to endpoint
        HandleStateOpened();
        break;
      }
      case kControlChannelStateEstablished: {
        // Start keep alive
        HandleStateEstablished();
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

  void HandleStateOpened() {
    // Send hello message to server
    auto hello_msg = protocol::HelloMessage("barbarossa@test");
    json j = hello_msg;
    endpoint_.Send(j.dump());

    // Start a thread that waits for the reply or timeout
    std::thread(
        [&](std::chrono::seconds timeout) {
          spdlog::debug("control_channel: Wait for hello message reply.");

          std::unique_lock<std::mutex> lock(hello_message_reply_mutex_);
          if (hello_message_reply_.wait_for(lock, timeout) ==
              std::cv_status::timeout) {
            spdlog::debug("control_channel: Timeout hello message reply.");
            RaiseEvent(kControlChannelEventOnTimeout,
                       "HELLO");  // TODO(DGL) Replace string
          }
        },
        std::chrono::seconds(4))
        .detach();  // TODO(DGL) Replace with a settings variable
  }

  void HandleStateEstablished() {
    // Start the hearbeat
  }

  // void HandleMessage(const std::string& payload);

  // int EstablishSession();
  // void WaitForHelloReplyOrDie();

  // void RaiseOnTimeoutEvent(protocol::MessageTypes msg_type);

  // void WaitForPongMessageOrDie();
};

}  // namespace barbarossa::controlchannel::v1

#endif  // CONTROLCHANNEL_HPP_
