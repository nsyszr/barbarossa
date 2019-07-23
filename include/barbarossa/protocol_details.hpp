#ifndef BARBAROSSA_PROTOCOL_DETAILS_HPP__
#define BARBAROSSA_PROTOCOL_DETAILS_HPP__

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace barbarossa::controlchannel::v1::protocol {

namespace welcomedetails {
// The representation of the websocket hello message (message type 1).
class WelcomeDetails {
  int session_timeout_;
  int ping_interval_;
  int pong_timeout_;
  std::string events_topic_;

  auto tie() const {
    return std::tie(session_timeout_, ping_interval_, pong_timeout_,
                    events_topic_);
  }

  friend void to_json(json& j, const WelcomeDetails& d);
  friend void from_json(const json& j, WelcomeDetails& d);

 public:
  WelcomeDetails()
      : session_timeout_(120), ping_interval_(104), pong_timeout_(16) {}
  WelcomeDetails(int session_timeout, int ping_interval, int pong_timeout,
                 const std::string& events_topic)
      : session_timeout_(session_timeout),
        ping_interval_(ping_interval),
        pong_timeout_(pong_timeout),
        events_topic_(events_topic) {}

  inline bool operator==(const WelcomeDetails& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto session_timeout() -> int { return session_timeout_; }
  auto ping_interval() -> int { return ping_interval_; }
  auto pong_timeout() -> int { return pong_timeout_; }
  auto events_topic() -> const std::string& { return events_topic_; }

  // mutators
  void set_session_timeout(int session_timeout) {
    session_timeout_ = session_timeout;
  }
  void set_ping_interval(int ping_interval) { ping_interval_ = ping_interval; }
  void set_pong_timeout(int pong_timeout) { pong_timeout_ = pong_timeout; }
  void set_events_topic(const std::string& events_topic) {
    events_topic_ = events_topic;
  }
};

inline void to_json(json& j, const WelcomeDetails& d) {
  j["session_timeout"] = d.session_timeout_;
  j["ping_interval"] = d.ping_interval_;
  j["pong_max_wait_time"] = d.pong_timeout_;
  j["events_topic"] = d.events_topic_;
}  // namespace welcomedetails

inline void from_json(const json& j, WelcomeDetails& d) {
  d.session_timeout_ = j.at("session_timeout").get<int>();
  d.ping_interval_ = j.at("ping_interval").get<int>();
  d.pong_timeout_ = j.at("pong_max_wait_time").get<int>();
  d.events_topic_ = j.at("events_topic").get<std::string>();
}

}  // namespace welcomedetails

}  // namespace barbarossa::controlchannel::v1::protocol

#endif  // BARBAROSSA_PROTOCOL_DETAILS_HPP__
