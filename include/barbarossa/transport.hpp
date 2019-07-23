// Copyright (c) 2018 by nsyszr.io.
// Author: dgl

#ifndef BARBAROSSA_TRANSPORT_HPP_
#define BARBAROSSA_TRANSPORT_HPP_

#include <string>

#include "barbarossa/message.hpp"

namespace barbarossa::controlchannel {

class NetworkError : public std::exception {
 public:
  explicit NetworkError(const std::string& message) : message_(message) {}

  const char* what() const throw() { return message_.c_str(); }

 private:
  std::string message_;
};

class Transport {
 public:
  virtual void Connect() = 0;
  virtual void Disconnect() = 0;
  virtual bool IsConnected() const = 0;
  virtual void SendMessage(Message&& message) = 0;
  //  virtual void Attach(
  //          const std::shared_ptr<wamp_transport_handler>& handler) = 0;
  virtual bool HasHandler() const = 0;
};

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_TRANSPORT_HPP_
