#ifndef ZMQUTILS_HPP_
#define ZMQUTILS_HPP_

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
  zmq::socket_t socket(context, ZMQ_PAIR);
  socket.bind(addr);
  return socket;
}

inline bool SendString(zmq::socket_t& socket, const std::string& string) {
  zmq::message_t message(string.size());
  memcpy(message.data(), string.data(), string.size());
  return socket.send(message);
}

inline std::string RecvString(zmq::socket_t& socket,
                              zmq::recv_flags flags = zmq::recv_flags::none) {
  zmq::message_t message;
  socket.recv(&message, static_cast<int>(flags));
  return std::string(static_cast<char*>(message.data()), message.size());
}

}  // namespace barbarossa::zmqutils

#endif  // ZMQUTILS_HPP_