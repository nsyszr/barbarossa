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

inline Session::Session(asio::io_service& io_service)
    : io_service_(io_service),
      state_(SessionState::kClosed),
      running_(false),
      session_id_(0),
      request_timeout_(kSessionDefaultRequestTimeout) {}

inline void Session::Start() {
  spdlog::info("session start called");

  // We use dispatch to synchronously send the message in the current thread.
  // After execution we can expect that the message is on the wire.
  auto weak_self = std::weak_ptr<Session>(shared_from_this());
  io_service_.dispatch([&]() {
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
  });

  auto started_future = started_.get_future();
  return started_future.get();
}

inline uint32_t Session::Join(const std::string& realm) {
  spdlog::info("session join started");

  Message hello_msg(2);
  hello_msg.SetField<MessageType>(0, MessageType::kHello);
  hello_msg.SetField<std::string>(1, realm);

  // Set the current state to ESTABLISHING
  EnsureState(SessionState::kEstablishing);

  // We use dispatch to synchronously send the message in the current thread.
  // After execution we can expect that the message is on the wire.
  auto weak_self = std::weak_ptr<Session>(shared_from_this());
  io_service_.dispatch([&]() {
    auto shared_self = weak_self.lock();
    if (!shared_self) {
      return;
    }

    if (session_id_) {
      joined_.set_exception(std::make_exception_ptr(
          std::logic_error(ErrorMessages::kSessionAlreadyJoined)));
      return;
    }

    try {
      transport_->SendMessage(std::move(hello_msg));
    } catch (std::exception& e) {
      joined_.set_exception(std::make_exception_ptr(std::copy_exception(e)));
    }
  });

  /*
  // Wait for the response or handle timeout
  if (joined_future.wait_for(request_timeout_) ==
      std::future_status::timeout) {
    FailAndThrowProtocolError("timeout");
  }
  */

  auto joined_future = joined_.get_future();
  return joined_future.get();
}

inline void Session::OnAttach(const std::shared_ptr<Transport>& transport) {
  if (transport_) {
    throw std::logic_error(ErrorMessages::kSessionTransportAlreadyAttached);
  }

  // TODO(DGL) It's mentioned that this is never possible because you cannot
  //           start a session without having an attached transport ???
  // assert(running)

  transport_ = transport;
}

inline void Session::OnDetach(const std::string& reason) {
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
      // ProcessPongMessage
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
  spdlog::debug("session change state from {} to {}", ToString(state_),
                ToString(state));
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

    throw ProtocolError(
        "client received WELCOME message but session already established");
  }

  // Set the current state to ESTABLISHED
  EnsureState(SessionState::kEstablished);

  session_id_ = message.Field<uint32_t>(1);
  joined_.set_value(session_id_);
}

}  // namespace barbarossa::controlchannel
