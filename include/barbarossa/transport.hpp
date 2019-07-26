// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#ifndef BARBAROSSA_TRANSPORT_HPP_
#define BARBAROSSA_TRANSPORT_HPP_

#include <memory>
#include <string>

#include "barbarossa/message.hpp"
#include "barbarossa/transport_handler.hpp"

namespace barbarossa::controlchannel {

class Transport {
 public:
  virtual void Connect() = 0;
  virtual void Disconnect() = 0;
  virtual bool IsConnected() const = 0;
  virtual void SendMessage(Message&& message) = 0;
  virtual void Attach(const std::shared_ptr<TransportHandler>& handler) = 0;
  virtual void Detach() = 0;
  virtual bool HasHandler() const = 0;
};

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_TRANSPORT_HPP_
