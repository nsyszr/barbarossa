// Copyright (c) 2018 by nsyszr.io.
// Author: dgl

#ifndef BARBAROSSA_MESSAGE_HANDLER_HPP_
#define BARBAROSSA_MESSAGE_HANDLER_HPP_

#include <functional>

#include "protocol.hpp"

namespace barbarossa::controlchannel::v1 {

// using MessageRequest = std::tuple<protocol::MessageTypes, int32_t>;

/*class MessageHandler {
 public:
  MessageHandler(std::function<void()> handler)
      : handler_(handler), next_(nullptr) {}

  auto request_id() -> int32_t { return request_id_; }
  void set_request_id(int32_t request_id) { request_id_ = request_id; }

 private:
  std::function<void()> handler_;
  MessageHandler *next_;
  protocol::MessageTypes message_type_;
  int32_t request_id_;
};*/

class WelcomeMessageHandler {
 public:
  void operator()() {}
};

}  // namespace barbarossa::controlchannel::v1

#endif  // BARBAROSSA_MESSAGE_HANDLER_HPP_
