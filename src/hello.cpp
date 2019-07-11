#include <iostream>
#include <string>

#include "protocol.hpp"

#include "nlohmann/json.hpp"

namespace details {
class Details {
 public:
  Details(const std::string& serial) : serial_(serial) {}

 private:
  friend void to_json(json& j, const Details&);

  std::string serial_;
};

void to_json(json& j, const Details& details) {
  j["serial_number"] = details.serial_;
}

}  // namespace details

int main() {
  using json = nlohmann::json;
  using namespace iotcore::devicecontrol::v1::protocol;
  using iotcore::devicecontrol::v1::protocol::placeholders::Placeholder;

  /*auto j = json::parse(
      "[2, 293847, {\"session_timeout\": 3600, \"ping_interval\": 3584, "
      "\"pong_max_wait_time\": 16, \"events_topic\": \"devices::events\"}]");
  std::cout << j << std::endl; */


  auto msg = hellomessage::HelloMessage{"test@test"};
  json j = msg;

  auto msg2 = hellomessage::HelloMessage<details::Details>{
      "test2@test2", details::Details{"1234"}};
  json j2 = msg2;

  auto j3 = json::parse("[2, \"test3@test3\", {}]");
  auto msg3 = j3.get<hellomessage::HelloMessage<json>>();

  std::cout << j << std::endl;
  std::cout << j2 << std::endl;
  std::cout << j3 << std::endl;

  std::cout << msg3.realm() << ", " << msg3.details() << std::endl;

  return 0;
}
