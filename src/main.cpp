#include <csignal>
#include <iostream>
#include <string>

#include "barbarossa/globals.hpp"
#include "barbarossa/websocket.hpp"
#include "barbarossa/zmq_utils.hpp"
#include "spdlog/spdlog.h"

namespace zmqutils = barbarossa::zmqutils;

/* namespace details {
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

}  // namespace details */


void RunWithTimeout(zmq::context_t& context) {
  spdlog::debug("Thread started.");

  // TODO(DGL) Fix hardcoded timeout
  long timeout_ms = 16000;

  try {
    /* zmq::socket_t wait_socket(*context, ZMQ_PAIR);
    wait_socket.bind("inproc://runwithtimeout"); */
    auto socket = zmqutils::Bind(context, "inproc://runwithtimeout", ZMQ_PAIR);

    // while (1) {
    //  Poll socket for a reply, with timeout
    zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};
    spdlog::debug("Poll queue.");
    zmq::poll(&items[0], 1, timeout_ms);

    //  If we got a reply, we can leave the thread without any response
    if (items[0].revents & ZMQ_POLLIN) {
      // Receive the message even if we don't handle it. This ensures that the
      // queue is empty.
      spdlog::debug("Message is in queue.");

      /*zmq::message_t msg;
      socket.recv(&msg, 0);  // zmq::recv_flags::none*/
      auto msg = zmqutils::RecvString(socket);

      spdlog::debug("Got a valid signal '{}'. Exit thread.", msg);

      return;  // Exit
    }
  } catch (zmq::error_t& e) {
    spdlog::error("ZMQ error: {}", e.what());
  }

  spdlog::debug("Thread expired.");
}

void ListenEvents(zmq::context_t& context) {
  spdlog::debug("Start ListenEvents thread.");

  auto event_socket = zmqutils::Bind(context, "inproc://events", ZMQ_PAIR);
  // auto quit_socket = zmqutils::Bind(context, "inproc://quit", ZMQ_PAIR);

  /*zmq::pollitem_t items[] = {{event_socket, 0, ZMQ_POLLIN, 0},
                             {quit_socket, 0, ZMQ_POLLIN, 0}};*/

  while (true) {
    /*zmq::poll(&items[0], 2, -1);  // Without timeout

    if (items[0].revents & ZMQ_POLLIN) {
      auto msg = zmqutils::RecvString(event_socket);
      spdlog::info("event: {}", msg);
    }
    if (items[1].revents & ZMQ_POLLIN) {
      auto msg = zmqutils::RecvString(quit_socket);
      spdlog::info("quit: {}", msg);
      spdlog::info("Exit ListenEvents");
      return;
    }*/

    try {
      auto msg = zmqutils::RecvString(event_socket);
      spdlog::debug("Received an event: {}", msg);
    } catch (zmq::error_t& e) {
      spdlog::warn("Failed to receive a message: {}", e.what());
    }
    if (barbarossa::gQuitSignal == SIGINT ||
        barbarossa::gQuitSignal == SIGTERM) {
      spdlog::info("Quit signal is set. Exit ListenEvents thread.");
      return;
    }
  }
}

void SendEvents(zmq::context_t& context) {
  using namespace std::chrono_literals;

  spdlog::debug("Start SendEvents thread.");

  auto event_socket = zmqutils::Connect(context, "inproc://events", ZMQ_PAIR);

  for (int i = 0; i < 5; i++) {
    std::this_thread::sleep_for(2s);

    std::ostringstream ss;
    ss << "EVENT " << (i + 1);
    try {
      zmqutils::SendString(event_socket, ss.str());
    } catch (zmq::error_t& e) {
      spdlog::warn("Failed to send message: {}", e.what());
    }
    if (barbarossa::gQuitSignal == SIGINT ||
        barbarossa::gQuitSignal == SIGTERM) {
      spdlog::info("Quit signal is set. Exit SendEvents thread.");
      return;
    }
  }

  spdlog::debug("Exit SendEvents thread.");
}

int main(/*int argc, char* argv[]*/) {
  spdlog::set_level(spdlog::level::debug);

  // using json = nlohmann::json;
  // using namespace barbarossa::controlchannel::v1::protocol;

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

  /*std::string uri = argv[1];
  std::cout << "Connecting " << uri << std::endl;
  WebsocketEndpoint endpoint;

  endpoint.Run(uri);*/

  //
  // Test signalling between threads including timeout for thread
  //
  /* std::thread t(&RunWithTimeout, std::ref(context));

  std::this_thread::sleep_for(17s);

  try {
    spdlog::debug("Connecting queue.");
    auto socket =
        zmqutils::Connect(context, "inproc://runwithtimeout", ZMQ_PAIR);

    spdlog::debug("Sending message.");
    zmqutils::SendString(socket, "QUIT");
  } catch (zmq::error_t& e) {
    spdlog::error("Failed to send message: {}", e.what());
  } catch (const std::exception& e) {
    spdlog::error("Failed to send message: {}", e.what());
  }

  t.join();*/

  //
  // Test request reply for events
  //
  // Install signal handler
  // std::signal(SIGINT, barbarossa::SignalHandler);
  // std::signal(SIGTERM, barbarossa::SignalHandler);
  if (!barbarossa::InstallSignalHandler()) {
    spdlog::critical("Could not install signal handlers.");
    exit(1);
  }

  // Start thread
  std::thread t1(&ListenEvents, std::ref(barbarossa::gInProcContext));
  std::thread t2(&SendEvents, std::ref(barbarossa::gInProcContext));

  // auto quit_socket = zmqutils::Connect(context, "inproc://quit", ZMQ_PAIR);
  // zmqutils::SendString(quit_socket, "QUIT");

  t2.join();
  t1.join();

  return 0;
}
