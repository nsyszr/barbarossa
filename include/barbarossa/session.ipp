// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is slightly rewritten based on the Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.
//
// Session Flow
// ============
// - Initial state is CLOSED
// - Join()
//   - Send HELLO message
//   - State is ESTABLISHING
// - OnMessage(WELCOME)
//   - State is ESTABLSIHED
// - OnMessage(ABORT)
//   - Throw AbortError
// - OnMessage(others)
//   - Throw ProtocolError
// - Didn't receive WELCOME message within timeout
//   - Throw TimeoutError
// - Network / Transport layer issues
//   - Throw NetworkError


#include "barbarossa/errors.hpp"
#include "barbarossa/message_type.hpp"
#include "spdlog/spdlog.h"

namespace barbarossa::controlchannel {

inline Session::Session(asio::io_context& io_context)
    : io_context_(io_context),
      state_(SessionState::kClosed),
      running_(false),
      session_id_(0),
      request_timeout_(kSessionDefaultRequestTimeout) {}

inline Session::~Session() {
  if (hearbeat_thread_.joinable()) {
    spdlog::debug("wait for heartbeat thread to finish");
    hearbeat_thread_.join();
  }
}

inline void Session::Start() {
  spdlog::info("session start called");

  // We use dispatch to synchronously send the message in the current thread.
  // After execution we can expect that the message is on the wire.
  auto self(shared_from_this());
  asio::dispatch(io_context_, [this, self]() {
    if (running_) {
      started_.set_exception(std::make_exception_ptr(
          std::logic_error(ErrorMessages::kSessionAlreadyJoined)));
      return;
    }

    if (!transport_) {
      started_.set_exception(std::make_exception_ptr(NoTransportError()));
      return;
    }

    running_ = true;
    started_.set_value();
  });

  /*auto weak_self = std::weak_ptr<Session>(shared_from_this());
  io_context_.dispatch([&]() {
    auto shared_self = weak_self.lock();
    if (!shared_self) {
      return;
    }

    if (running_) {
      started_.set_exception(std::make_exception_ptr(
          std::logic_error(ErrorMessages::kSessionAlreadyJoined)));
      return;
    }

    if (!transport_) {
      started_.set_exception(std::make_exception_ptr(NoTransportError()));
      return;
    }

    running_ = true;
    started_.set_value();
  });*/

  auto started_future = started_.get_future();
  return started_future.get();
}

inline uint32_t Session::Join(const std::string& realm) {
  spdlog::info("session join started");

  Message hello_message(2);
  hello_message.SetField<MessageType>(0, MessageType::kHello);
  hello_message.SetField<std::string>(1, realm);

  // Set the current state to ESTABLISHING
  EnsureState(SessionState::kEstablishing);

  // We use dispatch to synchronously send the message in the current thread.
  // After execution we can expect that the message is on the wire.
  auto self(shared_from_this());
  asio::dispatch(io_context_, [this, self, &hello_message]() {
    if (session_id_) {
      joined_.set_exception(std::make_exception_ptr(
          std::logic_error(ErrorMessages::kSessionAlreadyJoined)));
      return;
    }

    try {
      // Note that in this case an established session isn't required.
      SendMessage(std::move(hello_message), false);
    } catch (std::exception& e) {
      joined_.set_exception(std::make_exception_ptr(e));
    }
  });

  auto joined_future = joined_.get_future();

  // Wait for the future until timeout
  if (joined_future.wait_for(std::chrono::seconds(16)) ==
      std::future_status::timeout) {
    throw TimeoutError();
  }

  return joined_future.get();
}

inline void Session::Leave() { stop_signal_.notify_all(); }

inline void Session::OnAttach(const std::shared_ptr<Transport>& transport) {
  if (transport_) {
    throw std::logic_error(ErrorMessages::kSessionTransportAlreadyAttached);
  }

  // TODO(DGL) It's mentioned that this is never possible because you cannot
  //           start a session without having an attached transport ???
  // assert(running)

  transport_ = transport;
}

inline void Session::OnDetach(const std::string& /*reason*/) {
  if (!transport_) {
    throw std::logic_error(ErrorMessages::kSessionTransportAlreadyDetached);
  }

  transport_.reset();
}

inline void Session::OnMessage(Message&& message) {
  spdlog::debug("session received message");

  if (message.Size() < 1) {
    throw ProtocolError(ErrorMessages::kInvalidMessageStructure);
  }

  auto message_type = static_cast<MessageType>(message.Field<int>(0));

  switch (message_type) {
    case MessageType::kHello:
      throw ProtocolError("client received HELLO message");
    case MessageType::kWelcome:
      ProcessWelcomeMessage(std::move(message));
      break;
    case MessageType::kAbort:
      // abort session, throw AbortError
      break;
    case MessageType::kPing:
      throw ProtocolError("client received PING message");
    case MessageType::kPong:
      ProcessPongMessage(std::move(message));
      break;
    case MessageType::kError:
      // ProcessErrorMessage
      break;
    case MessageType::kCall:
      // ProcessCallMessage
      // if operation is not registered return ERROR message with reason:
      // ERR_NO_SUCH_OPERATION
      break;
    case MessageType::kResult:
      throw ProtocolError("client received unsupported RESULT message");
    case MessageType::kPublish:
      // Handle publish by returning an ERROR message with reason:
      // ERR_NO_SUCH_TOPIC
      break;
      // throw ProtocolError("client received unsupported PUBLISH message");
    case MessageType::kPublished:
      // ProcessPublishedMessage
      break;
  }
}

inline void Session::EnsureState(SessionState state) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  spdlog::debug("session change state from {} to {}", state_, state);
  state_ = state;
}

inline SessionState Session::CurrentState() {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return state_;
}

inline void Session::SendMessage(Message&& message, bool session_established) {
  if (!transport_) {
    throw NoTransportError();
  }
  if (!transport_->IsConnected()) {
    throw NoTransportError();
  }
  if (session_established && !session_id_) {
    throw NoSessionError();
  }

  transport_->SendMessage(std::move(message));
}

inline void Session::ProcessWelcomeMessage(Message&& message) {
  if (CurrentState() != SessionState::kEstablishing) {
    // Received WELCOME message after session was established
    // SendMessage(abort_msg); reason: ERR_PROTOCOL_VIOLATION

    // TODO(DGL) the protocol error should be set on the promise!
    throw ProtocolError(
        "client received WELCOME message but session already established");
  }

  // Set the current state to ESTABLISHED
  EnsureState(SessionState::kEstablished);

  // Start heartbeat
  hearbeat_thread_ = std::thread(&Session::HearbeatController, this);

  session_id_ = message.Field<uint32_t>(1);
  joined_.set_value(session_id_);
}

inline void Session::ProcessPongMessage(Message&& message) {
  spdlog::debug("handle pong message");
  hearbeat_alive_.set_value();
}

inline void Session::HearbeatController() {
  spdlog::debug("hearbeat: controller thread start");

  while (true) {
    std::unique_lock<std::mutex> lock(stop_signal_mutex_);
    spdlog::debug("heartbeat: wait until sending ping or die.");

    // We send every given seconds a ping or we stop the hearbeat
    if (stop_signal_.wait_for(lock, std::chrono::seconds(16)) ==
        std::cv_status::no_timeout) {
      spdlog::info("heartbeat: received the die condition and terminate now.");

      // heartbeat.set_value();
      break;  // Exit the loop because stop_signal_ is set
    }

    Message ping_message(2);
    ping_message.SetField<MessageType>(0, MessageType::kPing);
    ping_message.SetField<json::object_t>(1, json::object());

    // Dispatch the ping message
    auto self(shared_from_this());
    asio::dispatch(io_context_, [this, self, &ping_message]() {
      try {
        // Note that in this case an established session isn't required.
        SendMessage(std::move(ping_message), true);
      } catch (std::exception& e) {
        hearbeat_alive_.set_exception(std::make_exception_ptr(e));
      }
    });

    // Wait for our pong message
    auto f = hearbeat_alive_.get_future();
    if (f.wait_for(std::chrono::seconds(4)) == std::future_status::timeout) {
      spdlog::error("heartbeat timeout");
      break;
    }

    // If everything is fine get() doesn't block, but can raise an exception
    // TODO(DGL) handle the exception because we're running inside a thread!
    f.get();

    // Reset the promise for the next pong
    hearbeat_alive_ = std::promise<void>();
  }

  spdlog::debug("heartbeat terminated");
}

}  // namespace barbarossa::controlchannel
