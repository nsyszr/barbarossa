// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_ZMQ_UTILS_HPP_
#define BARBAROSSA_ZMQ_UTILS_HPP_

#include <string>

#include "zmq.hpp"

namespace barbarossa::zmqutils {

inline zmq::socket_t Connect(zmq::context_t& context, const std::string& addr,
                             int type) {
  zmq::socket_t socket(context, type);
  socket.connect(addr);
  return socket;
}

inline zmq::socket_t Bind(zmq::context_t& context, const std::string& addr,
                          int type) {
  zmq::socket_t socket(context, type);
  socket.bind(addr);
  return socket;
}

inline bool SendString(zmq::socket_t& socket, const std::string& string) {
  zmq::message_t message(string.size());
  memcpy(message.data(), string.data(), string.size());

  auto result = socket.send(message, zmq::send_flags::none);
  return result ? true : false;
}

inline std::string RecvString(zmq::socket_t& socket,
                              zmq::recv_flags flags = zmq::recv_flags::none) {
  zmq::message_t message;
  socket.recv(message, flags);
  return std::string(static_cast<char*>(message.data()), message.size());
}

}  // namespace barbarossa::zmqutils

#endif  // BARBAROSSA_ZMQ_UTILS_HPP_
