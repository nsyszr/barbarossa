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
      session_id_(0),
      request_timeout_(kSessionDefaultRequestTimeout) {
  spdlog::debug("session: instance is created");
}

inline Session::~Session() {
  if (hearbeat_thread_.joinable()) {
    spdlog::debug("wait for heartbeat thread to finish");
    hearbeat_thread_.join();
  }
  spdlog::debug("session: instance is destroyed");
}

inline uint32_t Session::Join(const std::string& realm) {
  spdlog::debug("session: joining");

  auto hello_message = std::make_shared<Message>(Message(2));
  hello_message->SetField<MessageType>(0, MessageType::kHello);
  hello_message->SetField<std::string>(1, realm);

  // Set the current state to ESTABLISHING
  EnsureState(SessionState::kEstablishing);

  // We use dispatch to synchronously send the message in the current thread.
  // After execution we can expect that the message is on the wire.
  // auto self(shared_from_this());
  auto weak_self = std::weak_ptr<Session>(this->shared_from_this());

  asio::dispatch(io_context_, [=]() {
    spdlog::debug("session: join dispatching");
    auto shared_self = weak_self.lock();
    if (!shared_self) {
      return;
    }

    if (session_id_) {
      joined_.set_exception(std::make_exception_ptr(
          std::logic_error(ErrorMessages::kSessionAlreadyJoined)));
      spdlog::debug("session dispatch join is exited");
      return;
    }

    try {
      // Note that in this case an established session isn't required.
      SendMessage(std::move(*hello_message), false);
    } catch (std::exception& e) {
      joined_.set_exception(std::make_exception_ptr(e));
    }
    spdlog::debug("session: join dispatched");
  });

  auto joined_future = joined_.get_future();

  // Wait for the future until timeout
  if (joined_future.wait_for(std::chrono::seconds(16)) ==
      std::future_status::timeout) {
    throw TimeoutError();
  }

  return joined_future.get();
}

inline void Session::Leave() {
  // stop_signal_.notify_all();
  leaved_.set_value(true);

  // TODO(DGL) This call is perhaps critical because of depending background
  // threads. Check this!
  transport_->Disconnect();
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

inline void Session::Listen() {
  auto leaved_future = leaved_.get_future();
  leaved_future.get();
}

inline void Session::RegisterOperation(const std::string& operation,
                                       std::function<json(json&&)> fn) {
  operations_[operation] = fn;
}

inline void Session::OnDetach(const std::string& /*reason*/) {
  if (!transport_) {
    throw std::logic_error(ErrorMessages::kSessionTransportAlreadyDetached);
  }

  transport_.reset();
}

inline void Session::OnMessage(Message&& message) {
  spdlog::debug("session: message is received");

  if (message.Size() < 1) {
    // throw ProtocolError(ErrorMessages::kInvalidMessageStructure);
    leaved_.set_exception(std::make_exception_ptr(
        ProtocolError(ErrorMessages::kInvalidMessageStructure)));
    return;
  }

  auto message_type = static_cast<MessageType>(message.Field<int>(0));

  switch (message_type) {
    case MessageType::kHello:
      throw ProtocolError("client received HELLO message");
    case MessageType::kWelcome:
      ProcessWelcomeMessage(std::move(message));
      break;
    case MessageType::kAbort:
      ProcessAbortMessage(std::move(message));
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
      ProcessCallMessage(std::move(message));
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
  spdlog::debug("session: change state from {} to {}", state_, state);
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

  // Throws NetworkError
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

inline void Session::ProcessAbortMessage(Message&& message) {
  // TODO(DGL) Abort message spec has a 3rd field with reason. Add this to the
  //           exception, tool.
  // TODO(DGL) Should we close the connection here, too?
  auto reason = message.Field<std::string>(1);
  joined_.set_exception(std::make_exception_ptr(AbortError(reason)));
}

inline void Session::ProcessPongMessage(Message&&) {
  spdlog::debug("session pong message is handled");
  pong_received_.set_value(true);
}

inline void Session::ProcessCallMessage(Message&& message) {
  // TODO(DGL) Add message checking like length, et.c

  int32_t request_id = message.Field<int32_t>(1);
  auto operation = message.Field<std::string>(2);
  auto arguments = message.Field<json>(3);

  spdlog::debug("process call message: {}, {}, {}", request_id, operation,
                arguments);

  auto it = operations_.find(operation);
  if (it != operations_.end()) {
    auto result = it->second(std::move(arguments));

    spdlog::debug("session: call message processed: {}", result.dump());

    Message result_message(3);
    result_message.SetField<MessageType>(0, MessageType::kResult);
    result_message.SetField<int32_t>(1, request_id);
    result_message.SetField<json>(2, result);

    SendMessage(std::move(result_message));
  } else {
    spdlog::debug("call message failed");

    Message error_message(5);
    error_message.SetField<MessageType>(0, MessageType::kError);
    error_message.SetField<MessageType>(1, MessageType::kCall);
    error_message.SetField<int32_t>(2, request_id);
    error_message.SetField<std::string>(3, "ERR_NO_SUCH_OPERATION");
    error_message.SetField<json>(4, json::object());

    SendMessage(std::move(error_message));
  }
}

inline void Session::HearbeatController() {
  spdlog::debug("session: hearbeat controller is started");

  // auto leaved_future = leaved_.get_future();
  while (true) {
    std::unique_lock<std::mutex> lock(stop_signal_mutex_);
    spdlog::debug("session: heartbeat wait until sending ping or die.");

    // We send every given seconds a ping or we stop the hearbeat
    if (stop_signal_.wait_for(lock, std::chrono::seconds(20)) ==
        std::cv_status::no_timeout) {
      spdlog::info("heartbeat: received the die condition and terminate now.");

      // heartbeat.set_value();
      break;  // Exit the loop because stop_signal_ is set
    }

    auto ping_message = std::make_shared<Message>(2);
    ping_message->SetField<MessageType>(0, MessageType::kPing);
    ping_message->SetField<json::object_t>(1, json::object());

    // Dispatch the ping message
    auto weak_self = std::weak_ptr<Session>(this->shared_from_this());
    asio::dispatch(io_context_, [=]() {
      auto shared_self = weak_self.lock();
      if (!shared_self) {
        return;
      }

      try {
        SendMessage(std::move(*ping_message), true);
      } catch (const NetworkError& e) {
        spdlog::debug("session: heartbeat failed to send message");
        pong_received_.set_value(false);
        leaved_.set_exception(std::make_exception_ptr(e));
      }
    });

    spdlog::debug("session: heartbeat wait_for ping_received_future");

    // Wait for our pong message
    auto pong_received_future = pong_received_.get_future();

    // The future wait_for can raise an exception, e.g. if send message above
    // fails.
    if (pong_received_future.wait_for(std::chrono::seconds(4)) ==
        std::future_status::timeout) {
      spdlog::error("heartbeat timeout");
      // Set our heartbeat alive promise to false! This tells our routine that
      // a timeout happend!
      pong_received_.set_value(false);
    }

    try {
      // If everything is fine get() doesn't block, but can raise an exception.
      // If the future is false, then it's set above because of timeout!
      if (!pong_received_future.get()) {
        leaved_.set_exception(std::make_exception_ptr(TimeoutError()));
        break;
      }
    } catch (const NetworkError& e) {
      leaved_.set_exception(std::make_exception_ptr(e));
    }

    // Reset the promise for the next pong
    pong_received_ = std::promise<bool>();
  }

  spdlog::debug("session heartbeat controller is terminated");
}

}  // namespace barbarossa::controlchannel
