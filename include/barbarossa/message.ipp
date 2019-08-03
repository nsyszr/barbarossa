// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#include <cstdint>

namespace barbarossa::controlchannel {

inline Message::Message(std::size_t num_fields) : fields_(num_fields) {}

inline Message::Message(MessageFields&& fields) : fields_(std::move(fields)) {}

inline Message::Message(Message&& other) { fields_ = std::move(other.fields_); }

inline Message& Message::operator=(Message&& other) {
  if (this == &other) {
    return *this;
  }

  fields_ = std::move(other.fields_);

  return *this;
}

inline const json& Message::Field(std::size_t index) const {
  if (index >= fields_.size()) {
    throw std::out_of_range("invalid message field index");
  }

  return fields_[index];
}

template <typename Type>
inline Type Message::Field(std::size_t index) {
  if (index >= fields_.size()) {
    throw std::out_of_range("invalid message field index");
  }

  return fields_[index].get<Type>();
}

template <typename Type>
inline void Message::SetField(std::size_t index, const Type& type) {
  if (index >= fields_.size()) {
    throw std::out_of_range("invalid message field index");
  }

  fields_[index] = type;
}

inline std::size_t Message::Size() const { return fields_.size(); }

inline const Message::MessageFields& Message::Fields() const { return fields_; }

inline Message::MessageFields&& Message::Fields() { return std::move(fields_); }

}  // namespace barbarossa::controlchannel