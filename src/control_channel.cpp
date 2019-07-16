#include "barbarossa/control_channel.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include "barbarossa/globals.hpp"
#include "barbarossa/protocol.hpp"
#include "barbarossa/zmq_utils.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "zmq_addon.hpp"

using json = nlohmann::json;
namespace zmqutils = barbarossa::zmqutils;

namespace barbarossa::controlchannel::v1 {

void ControlChannel::HandleStates() {
  spdlog::debug("Handle state: {}", state_);

  switch (state_) {
    case kControlChannelStatePending: {
      // Initial state, open the websocket connection.
      con_.Connect();
      break;
    }
    case kControlChannelStateOpened: {
      // Send hello message
      if (EstablishSession() != 0) {
        // Error state
      }
      break;
    }
    case kControlChannelStateEstablished: {
      // Start keep alive
      std::thread(&ControlChannel::WaitForPongMessageOrDie, this).detach();
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

int ControlChannel::EstablishSession() {
  // TODO(DGL) Fix hardcoded realm
  auto hello_msg = protocol::HelloMessage("test@test");
  json j = hello_msg;

  // TODO(DGL) Replace this with ZMQ request/reply queue! The blocking websocket
  //           run handler waits for a message instead of a while sleep loop.
  if (int rc = con_.SendMessage(j.dump()) != 0) {
    // TODO(DGL) Log error
    spdlog::error("Failed to send hello message: {}", rc);
    return rc;
  }

  std::thread(&ControlChannel::WaitForHelloReplyOrDie, this).detach();

  return 0;
}

void ControlChannel::WaitForHelloReplyOrDie() {
  spdlog::debug("Wait for welcome message or die thread started.");

  // TODO(DGL) Fix hardcoded timeout
  long timeout_ms = 16000;

  /*zmq::context_t context(1);
  zmq::socket_t wait_socket(context, ZMQ_REP);
  wait_socket.bind("inproc://waitforwelcomemessageordie");*/
  auto socket = zmqutils::Bind(barbarossa::gInProcContext,
                               "inproc:://helloreply", ZMQ_PAIR);

  // while (1) {
  //  Poll socket for a reply, with timeout
  zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};
  zmq::poll(&items[0], 1, timeout_ms);

  //  If we got a reply, we can leave the thread without any response
  if (items[0].revents & ZMQ_POLLIN) {
    // Receive the message even if we don't handle it. This ensures that the
    // queue is empty.
    /*zmq::message_t msg;
    wait_socket.recv(&msg, 0);  // zmq::recv_flags::none*/
    auto msg = zmqutils::RecvString(socket);

    spdlog::debug("Received a welcome message signal: {}", msg);
    return;  // Exit successfully
  }

  // We didn't received a hello message within timeout period. Notify the
  // control channel event loop.
  RaiseOnTimeoutEvent(protocol::kMessageTypeHello);

  spdlog::warn("Wait for welcome message or die thread expired.");
  // return;  // Exit the loop
  //}
}

void ControlChannel::RaiseEvent(ControlChannelEvents event) {
  switch (event) {
    case kControlChannelEventOnOpen:
    case kControlChannelEventOnClose:
    case kControlChannelEventOnFailed:
    case kControlChannelEventOnInterrupt: {
      std::lock_guard<std::mutex> lock(raise_event_mutex_);
      auto socket = zmqutils::Connect(barbarossa::gInProcContext,
                                      "inproc://events", ZMQ_PAIR);

      zmq::message_t msg(event);
      socket.send(msg, zmq::send_flags::none);

      break;  // lock will be released when out of this scope
    }
    case kControlChannelEventOnMessage:
    case kControlChannelEventOnTimeout: {
      throw InvalidOperationError();
    }
  }
}

void ControlChannel::RaiseEvent(ControlChannelEvents event, const json &j) {
  switch (event) {
    case kControlChannelEventOnOpen:
    case kControlChannelEventOnClose:
    case kControlChannelEventOnFailed:
    case kControlChannelEventOnInterrupt: {
      throw InvalidOperationError();
    }
    case kControlChannelEventOnMessage:
    case kControlChannelEventOnTimeout: {
      std::lock_guard<std::mutex> lock(raise_event_mutex_);
      auto socket = zmqutils::Connect(barbarossa::gInProcContext,
                                      "inproc://events", ZMQ_PAIR);

      zmq::multipart_t request;
      request.addtyp<ControlChannelEvents>(event);
      request.addstr(j.dump());

      request.send(socket);

      break;  // lock will be released when out of this scope
    }
  }
}

void ControlChannel::RaiseOnTimeoutEvent(protocol::MessageTypes msg_type) {
  RaiseEvent(kControlChannelEventOnTimeout, msg_type);
}

void ControlChannel::WaitForPongMessageOrDie() {
  spdlog::debug("Wait for ping message or die thread started.");

  // TODO(DGL) Fixe hardcoded timeout
  long ping_interval_ms = 16000;
  long pong_timeout_ms = 4000;

  zmq::context_t context(1);
  zmq::socket_t wait_socket(context, ZMQ_REP);
  wait_socket.bind("inproc://waitforpongmessageordie");

  while (true) {
    // Wait until it's time to send a ping message. If we receive a message
    // within timeout period the control channel is interrupted.
    zmq::pollitem_t items[] = {{wait_socket, 0, ZMQ_POLLIN, 0}};
    zmq::poll(&items[0], 1, ping_interval_ms);

    // If we receive a message the control channel signals a quit.
    if (items[0].revents & ZMQ_POLLIN) {
      // Read the message and decide the next step
      zmq::message_t msg;
      wait_socket.recv(&msg, 0);  // zmq::recv_flags::none
      auto payload = std::string(static_cast<char *>(msg.data()), msg.size());

      spdlog::debug("Wait for ping message or die thread got a signal: {}",
                    payload);

      if (payload != "QUIT") {
        // TODO(DGL): Log a warning because of unexpected signal
        spdlog::warn(
            "Wait for ping message or die thread got an unexpected signal '{}' "
            "but expected 'QUIT'.",
            payload);
      }
      return;  // Exit
    }

    // Send a ping message
    auto ping_msg = protocol::PingMessage();
    json j = ping_msg;
    if (con_.SendMessage(j.dump()) != 0) {
      // TODO(DGL) Log an error
      // Failed to send a ping message. We stop now!
      spdlog::error(
          "Wait for ping message or die thread stops. Could not send ping "
          "message: {}",
          1);
      return;
    }

    // Let's wait for a pong message within given timeout
    zmq::poll(&items[0], 1, pong_timeout_ms);

    // We got a message back. If it's not a reset timeout message we stop our
    // thread.
    if (items[0].revents & ZMQ_POLLIN) {
      // Read the message and decide the next step
      zmq::message_t msg;
      wait_socket.recv(&msg, 0);  // zmq::recv_flags::none
      auto payload = std::string(static_cast<char *>(msg.data()), msg.size());

      spdlog::debug("Wait for ping message or die thread got a signal: {}",
                    payload);

      if (payload != "RESET") {
        // TODO(DGL) Log a warning in case of not QUIT
        spdlog::debug(
            "Wait for ping message or die thread got a signal {} and exits.",
            payload);
        return;
      }
      // We continue looping because we received a pong message
    } else {
      // We didn't received a pong message within timeout period. We notify the
      // control channel and stop the thread.
      zmq::socket_t controlchannel_socket(context, ZMQ_REQ);
      controlchannel_socket.connect("inproc://controlchannel");

      zmq::multipart_t request;
      request.addtyp<ControlChannelEvents>(kControlChannelEventOnTimeout);
      request.addtyp<protocol::MessageTypes>(protocol::kMessageTypePing);

      request.send(controlchannel_socket);

      spdlog::warn("Wait for ping message or die thread expired.");
      return;
    }
  }
}

void ControlChannel::TransitionStateTo(ControlChannelStates state) {
  spdlog::warn("Change state from {} to {}.", state_, state);
  state_ = state;
  HandleStates();
}

void ControlChannel::Run() {
  socket_.bind("inproc://controlchannel");
  con_.Connect();

  while (1) {
    zmq::multipart_t request(socket_);

    // TODO(DGL) Handle exception!
    auto event = request.poptyp<ControlChannelEvents>();
    spdlog::debug("Received an event: {}", event);

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
      case kControlChannelEventOnFailed: {
        TransitionStateTo(kControlChannelStateClosed);
        break;
      }
      case kControlChannelEventOnMessage: {
        auto msg = request.popstr();
        HandleMessage(msg);
        break;
      }
      case kControlChannelEventOnTimeout: {
        auto msg_type = request.poptyp<protocol::MessageTypes>();
        if (msg_type == protocol::kMessageTypeHello) {
          // No respond to hello
        } else if (msg_type == protocol::kMessageTypePing) {
          // Change state
          TransitionStateTo(kControlChannelStateExpired);
        }
      }
    }
  }
}

void ControlChannel::HandleMessage(const std::string_view &s) {
  // TODO(DGL) Handle exception!
  auto msg = protocol::Parse(s);

  spdlog::debug("Handle message of type: {}", msg.GetMessageType());

  switch (msg.GetMessageType()) {
    case protocol::kMessageTypeWelcome: {
      // Signal WaitForWelcomeMessageOrDie
      zmq::socket_t wait_socket(context_, ZMQ_REQ);
      wait_socket.connect("inproc://waitforwelcomemessageordie");
      zmq::multipart_t request("QUIT");
      request.send(wait_socket);

      // Change state
      // TODO(DGL) We shouldnt do this here, return state instead
      TransitionStateTo(kControlChannelStateEstablished);

      break;
    }
    case protocol::kMessageTypeAbort: {
      auto abort_msg = msg.Get<protocol::abortmessage::AbortMessage>();
      if (state_ == kControlChannelStateOpened) {
        // Signal WaitForWelcomeMessageOrDie
        // TODO(DGL) Duplicate code see above
        zmq::socket_t wait_socket(context_, ZMQ_REQ);
        wait_socket.connect("inproc://waitforwelcomemessageordie");
        zmq::multipart_t request("QUIT");
        request.send(wait_socket);

        // Change state
        // TODO(DGL) We shouldnt do this here, return state instead
        TransitionStateTo(kControlChannelStateRefused);

        // Our hello message was aborted
      } else {
        // We received an abort because something went wrong
      }
      break;
    }
    case protocol::kMessageTypeError: {
      // Error message received, lookup request table for response handling
      auto error_msg = msg.Get<protocol::errormessage::ErrorMessage>();
      break;
    }
    case protocol::kMessageTypePong: {
      // Signal WaitForPongMessageOrDie
      // TODO(DGL) Similar code to waitforwelcomemessageordie
      zmq::socket_t wait_socket(context_, ZMQ_REQ);
      wait_socket.connect("inproc://waitforpongemessageordie");
      zmq::multipart_t request("RESET");
      request.send(wait_socket);

      break;
    }
    case protocol::kMessageTypeCall: {
      // Do something useful
      break;
    }
    case protocol::kMessageTypePublished: {
      // Do something useful
      break;
    }
    case protocol::kMessageTypeHello:
    case protocol::kMessageTypePing:
    case protocol::kMessageTypeResult:
    case protocol::kMessageTypePublish: {
      // not supported messages
      spdlog::warn("Do not handle Unsupported message of type: {}",
                   msg.GetMessageType());
      break;
    }
  }
}

}  // namespace barbarossa::controlchannel::v1