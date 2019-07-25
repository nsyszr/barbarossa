// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#include "barbarossa/message_type.hpp"

namespace barbarossa::controlchannel {

inline Session::Session(asio::io_service& io_service)
    : io_service_(io_service),
      state_(SessionState::kClosed),
      session_id_(0),
      request_timeout_(kSessionDefaultRequestTimeout) {}

inline uint32_t Session::Join(const std::string& realm) {
  Message hello_msg(2);
  hello_msg.SetField<MessageType>(0, MessageType::kHello);
  hello_msg.SetField<std::string>(1, realm);

  // Set the current state to ESTABLISHING
  EnsureState(SessionState::kEstablishing);

  // We use dispatch to synchronously send the message in the current thread.
  // After execution we can expect that the message is on the wire.
  auto weak_self = std::weak_ptr<Session>(shared_from_this());
  io_service_.dispatch([=]() {
    auto shared_self = weak_self.lock();
    if (!shared_self) {
      return;
    }

    // if (session_id_)

    try {
      transport_->SendMessage(std::move(*hello_msg));
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
    // throw protocol_error
  }

  // TODO(DGL) It's mentioned that this is never possible because you cannot
  //           start a session without having an attached transport ???
  // assert(running)

  transport_ = transport;
}

inline void Session::OnDetach(const std::string& reason) {
  if (!transport_) {
    // throw protocol_error
  }

  transport_.reset();
}

inline void Session::OnMessage(Message&& message) {
  if (message.Size() < 1) {
    // throw protocol_error
  }
  // get message type
  // switch message type
}

inline void Session::EnsureState(SessionState state) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  state_ = state;
}

inline void Session::ProcessWelcomeMessage(Message&& message) {
  // Set the current state to ESTABLISHED
  EnsureState(SessionState::kEstablished);

  session_id_ = message.Field<uint32_t>(1);
  joined_.set_value(session_id);
}

}  // namespace barbarossa::controlchannel
