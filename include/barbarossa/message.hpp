// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#ifndef BARBAROSSA_MESSAGE_HPP_
#define BARBAROSSA_MESSAGE_HPP_

#include <cstdint>
#include <vector>

#include "nlohmann/json.hpp"

namespace barbarossa::controlchannel {

using json = nlohmann::json;

class Message {
 public:
  using MessageFields = std::vector<json>;

  explicit Message(std::size_t num_fields);
  explicit Message(MessageFields&& fields);

  Message& operator=(const Message& other) = delete;
  Message& operator=(Message&& other);

  const json& Field(std::size_t index) const;
  template <typename Type>
  Type Field(std::size_t index);

  template <typename Type>
  void SetField(std::size_t index, const Type& type);

  std::size_t Size() const;

  const MessageFields& Fields() const;
  MessageFields&& Fields();

 private:
  MessageFields fields_;
};

}  // namespace barbarossa::controlchannel

#include "barbarossa/message.ipp"
#endif  // BARBAROSSA_MESSAGE_HPP_
