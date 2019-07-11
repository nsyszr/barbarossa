// Copyright (c) 2018 by nsyszr.io.
// Author: dgl

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

#include <string>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace iotcore::devicecontrol::v1::protocol {

enum MessageTypes {
  kMessageTypeInvalid = 0,
  kMessageTypeHello = 1,
  kMessageTypeWelcome = 2
};

namespace placeholders {

class Placeholder {};
void to_json(json& j, const Placeholder&) { j = json::object(); }

}  // namespace placeholders

namespace hellomessage {

using iotcore::devicecontrol::v1::protocol::placeholders::Placeholder;

template <typename T = json>
class HelloMessage {
 public:
  HelloMessage() {}
  HelloMessage(const std::string& realm) : realm_(realm) {}
  HelloMessage(const std::string& realm, const T& details)
      : realm_(realm), details_(details) {}

  // accessors
  const std::string& realm() { return realm_; }
  const T& details() { return details_; }

 private:
  template <typename T_>
  friend void to_json(json& j, const HelloMessage<T_>& msg);
  template <typename T_>
  friend void from_json(const json& j, HelloMessage<T_>& msg);

  std::string realm_;
  T details_;
};

template <typename T>
void to_json(json& j, const HelloMessage<T>& msg) {
  j.push_back(kMessageTypeHello);
  j.push_back(msg.realm_);
  j.push_back(msg.details_);
}

template <typename T>
void from_json(const json& j, HelloMessage<T>& msg) {
  // j.at("name").get_to(p.name);
  const auto realm = j[1];
  const auto details = j[2];
  msg.realm_ = realm;
  msg.details_ = details;
}

}  // namespace hellomessage

}  // namespace iotcore::devicecontrol::v1::protocol

#endif  // PROTOCOL_HPP_