#include "protocol.hpp"
#include "websocket.hpp"

#include "nlohmann/json.hpp"

#include <iostream>
#include <string>

namespace details {
class Details {
  std::string serial_;

  auto tie() const { return std::tie(serial_); }

  friend void to_json(json& j, const Details&);
  friend void from_json(const json& j, Details&);

 public:
  Details() {}
  Details(const std::string& serial) : serial_(serial){};

  inline bool operator==(const Details& rhs) const {
    return tie() == rhs.tie();
  }
};

void to_json(json& j, const Details& details) {
  j = json{{"serial_number", details.serial_}};
}

void from_json(const json& j, Details& details) {
  j.at("serial_number").get_to(details.serial_);
}

}  // namespace details

int main(int argc, char* argv[]) {
  using json = nlohmann::json;
  using namespace barbarossa::controlchannel::v1::protocol;

  /*// auto msg = hellomessage::HelloMessage{"test@test"};
  auto msg = HelloMessage("test@test");
  json j = msg;

  // auto msg2 = hellomessage::HelloMessage{"test2@test2"};
  auto msg2 = HelloMessage("test2@test2", details::Details{"5678"});
  // msg2.set_details(details::Details{"5678"});
  json j2 = msg2;

  auto j3 = json::parse("[2, \"test3@test3\"]");
  auto msg3 = j3.get<hellomessage::HelloMessage>();

  auto j4 = json::parse("[2, \"test3@test3\", {\"serial_number\": \"1234\"}]");
  auto msg4 = j4.get<hellomessage::HelloMessage>();

  std::cout << j << std::endl;
  std::cout << j2 << std::endl;
  std::cout << j3 << std::endl;
  std::cout << j4 << std::endl;

  std::cout << msg3.realm() << ", " << msg3.details() << std::endl;

  std::cout << msg4.realm() << ", " << msg4.details() << std::endl;
  auto details = msg4.details<details::Details>();*/

  // std::string uri = "ws://localhost:4001/devicecontrol/v1";
  std::string uri = argv[1];
  std::cout << "Connecting " << uri << std::endl;
  WebsocketEndpoint endpoint;

  endpoint.Run(uri);

  return 0;
}
