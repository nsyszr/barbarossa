// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#ifndef BARBAROSSA_SESSION_HPP_
#define BARBAROSSA_SESSION_HPP_

#include <cstdint>
#include <future>
#include <memory>
#include <string>

#include "asio.hpp"
#include "session_state.hpp"
#include "transport.hpp"
#include "transport_handler.hpp"

namespace barbarossa::controlchannel {

const std::chrono::seconds kSessionDefaultRequestTimeout{16};

class Session : public TransportHandler, std::enable_shared_from_this<Session> {
 public:
  explicit Session(asio::io_service& io_service);

  // Join sends the hello message and waits for response
  uint32_t Join(const std::string& realm);

 private:
  // Implement the transport handler interface
  void OnAttach(const std::shared_ptr<Transport>& transport);
  void OnDetach(const std::string& reason);
  void OnMessage(Message&& message);

  void EnsureState(SessionState state);

  void ProcessWelcomeMessage(Message&& message);

  asio::io_service& io_service_;
  SessionState state_;
  uint32_t session_id_;
  std::chrono::seconds request_timeout_;

  std::mutex state_mutex_;
  std::shared_ptr<Transport> transport_;
  std::promise<uint32_t> joined_;
};

}  // namespace barbarossa::controlchannel

#include "barbarossa/session.ipp"
#endif  // BARBAROSSA_SESSION_HPP_
