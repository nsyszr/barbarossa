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

#define ASIO_STANDALONE
#include "asio.hpp"
#include "barbarossa/session_state.hpp"
#include "barbarossa/transport.hpp"
#include "barbarossa/transport_handler.hpp"

namespace barbarossa::controlchannel {

const std::chrono::seconds kSessionDefaultRequestTimeout(16);

class Session : public TransportHandler,
                public std::enable_shared_from_this<Session> {
 public:
  // It's best practise to pass the asio::io_service as non-const reference.
  explicit Session(asio::io_service& io_service);  // NOLINT

  void Start();
  // Join sends the hello message and waits for response
  uint32_t Join(const std::string& realm);

 private:
  // Implement the transport handler interface
  void OnAttach(const std::shared_ptr<Transport>& transport);
  void OnDetach(const std::string& reason);
  void OnMessage(Message&& message);

  void EnsureState(SessionState state);
  SessionState CurrentState();

  void SendMessage(Message&& message, bool session_established = true);
  void ProcessWelcomeMessage(Message&& message);
  void HearbeatController();

  asio::io_service& io_service_;
  SessionState state_;
  bool running_;
  uint32_t session_id_;
  std::chrono::seconds request_timeout_;

  std::mutex state_mutex_;
  std::shared_ptr<Transport> transport_;
  std::promise<uint32_t> joined_;
  std::promise<void> started_;

  std::condition_variable stop_signal_;
  std::mutex stop_signal_mutex_;

  std::thread hearbeat_thread_;
};

}  // namespace barbarossa::controlchannel

#include "barbarossa/session.ipp"
#endif  // BARBAROSSA_SESSION_HPP_