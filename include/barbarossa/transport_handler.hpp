// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#ifndef BARBAROSSA_TRANSPORT_HANDLER_HPP_
#define BARBAROSSA_TRANSPORT_HANDLER_HPP_

#include <memory>
#include <string>

#include "barbarossa/message.hpp"
#include "barbarossa/transport.hpp"

namespace barbarossa::controlchannel {

class Transport;

class TransportHandler {
 public:
  virtual ~TransportHandler() = default;

  virtual void OnAttach(const std::shared_ptr<Transport>& transport) = 0;
  virtual void OnDetach(const std::string& reason) = 0;
  virtual void OnMessage(Message&& message) = 0;
};

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_TRANSPORT_HANDLER_HPP_
