// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#include <condition_variable>
#include <csignal>
#include <future>
#include <iostream>
#include <string>

#include "barbarossa/control_channel.hpp"
#include "barbarossa/globals.hpp"
#include "barbarossa/websocket_endpoint.hpp"
#include "barbarossa/websocket_transport.hpp"
#include "barbarossa/zmq_utils.hpp"
#include "spdlog/spdlog.h"

namespace zmqutils = barbarossa::zmqutils;

namespace {
std::function<void(int)> ShutdownHandler;
void SignalHandler(int signal) { ShutdownHandler(signal); }
}  // namespace

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

void SendPong(zmq::context_t& context) {
  using namespace std::chrono_literals;

  spdlog::debug("Start SendPong thread.");

  auto event_socket =
      zmqutils::Connect(context, "inproc://heartbeat", ZMQ_PAIR);

  std::this_thread::sleep_for(3s);

  try {
    zmqutils::SendString(event_socket, "PONG");
  } catch (zmq::error_t& e) {
    spdlog::warn("Failed to send message: {}", e.what());
  }
}

bool RequestPingPong(zmq::context_t& context) {
  spdlog::debug("RequestPingPong called");

  auto event_socket = zmqutils::Bind(context, "inproc://heartbeat", ZMQ_PAIR);
  try {
    auto msg = zmqutils::RecvString(event_socket);
    spdlog::debug("Received an event: {}", msg);
    return true;
  } catch (zmq::error_t& e) {
    spdlog::warn("Failed to receive a message: {}", e.what());
  }
  if (barbarossa::gQuitSignal == SIGINT || barbarossa::gQuitSignal == SIGTERM) {
    spdlog::info("Quit signal is set. Exit ListenEvents thread.");
  }
  return false;
}


int main(int argc, char* argv[]) {
  spdlog::set_level(spdlog::level::debug);

  if (argc <= 2) {
    std::cout << "Please provide the device control URI and device realm"
              << std::endl;
    return 1;
  }

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
  // if (!barbarossa::InstallSignalHandler()) {
  //  spdlog::critical("Could not install signal handlers.");
  //  exit(1);
  // }

  // std::thread t(&SendPong, std::ref(barbarossa::gInProcContext));

  /*std::future<bool> request =
      std::async(RequestPingPong, std::ref(barbarossa::gInProcContext));

  std::chrono::system_clock::time_point timeout_at =
      std::chrono::system_clock::now() + std::chrono::milliseconds(2000);

  if (request.wait_until(timeout_at) == std::future_status::ready) {
    std::cout << "Pong received with value: " << request.get() << std::endl;
  } else {
    std::cout << "Pong timeoute" << std::endl;
  }*/

  /*std::promise<bool> request;
  std::future<bool> request_result = request.get_future();
  std::thread(
      [](std::promise<bool> request) {
        auto socket = zmqutils::Bind(barbarossa::gInProcContext,
                                     "inproc://heartbeat", ZMQ_PAIR);

        zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};

        std::chrono::seconds timeout_s(4);
        zmq::poll(&items[0], 1, timeout_s);

        bool reply = false;
        if (items[0].revents & ZMQ_POLLIN) {
          auto msg = zmqutils::RecvString(socket);
          spdlog::info("Got message: {}", msg);
          reply = true;
        }

        request.set_value_at_thread_exit(reply);
      },
      std::move(request))
      .detach();

  std::cout << "Request reply: " << request_result.get() << std::endl;*/

  //
  // IMPORTANT CODE HERE FOR CONTROL CHANNEL
  //

  /*std::condition_variable stop_heartbeat;
  std::mutex stop_heartbeat_mutex;

  std::condition_variable stop_reply;
  std::mutex stop_reply_mutex;

  std::signal(SIGINT, SignalHandler);
  ShutdownHandler = [&](int signal) {
    spdlog::info("Shutting down server");
    std::unique_lock<std::mutex> lock(stop_heartbeat_mutex);
    stop_heartbeat.notify_one();

    std::unique_lock<std::mutex> lock2(stop_reply_mutex);
    stop_reply.notify_one();
  };

  std::thread heartbeat(
      [&](std::chrono::seconds ping_intervall,
          std::chrono::seconds pong_timeout) {
        while (true) {
          std::unique_lock<std::mutex> lock(stop_heartbeat_mutex);

          spdlog::debug("Wait until sending ping or die.");
          // We send every given seconds a ping or we stop the hearbeat
          if (stop_heartbeat.wait_for(lock, ping_intervall) ==
              std::cv_status::no_timeout) {
            spdlog::info("Terminate heartbeat. Received the stop condition.");
            break;  // Exit the loop because stop_heartbeat condition is set
          }

          // Send ping
          spdlog::debug("Sending ping.");
          auto ping_socket = zmqutils::Connect(barbarossa::gInProcContext,
                                               "inproc://ping", ZMQ_PAIR);
          zmqutils::SendString(ping_socket, "PING");

          // Create a async wait ping reply routine which returns a future
          auto request = std::async(
              [](std::chrono::seconds timeout) {
                auto socket = zmqutils::Bind(barbarossa::gInProcContext,
                                             "inproc://pong", ZMQ_PAIR);

                zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};

                zmq::poll(&items[0], 1, timeout);

                if (items[0].revents & ZMQ_POLLIN) {
                  auto msg = zmqutils::RecvString(socket);
                  spdlog::info("Got message: {}", msg);
                  return true;
                }

                return false;
              },
              pong_timeout);

          // Wait until we get a reply
          spdlog::debug("Waiting for pong.");
          bool reply = request.get();
          spdlog::debug("Pong result: {}");

          if (reply == false) {
            spdlog::warn("Terminate heartbeat. We didn't received a pong.");
            break;  // Exit the loop because ping wasn't replied within time.
          }
        }
      },
      std::chrono::seconds(16), std::chrono::seconds(4));


  std::thread reply([&]() {
    std::chrono::seconds timeout(1);
    auto socket =
        zmqutils::Bind(barbarossa::gInProcContext, "inproc://ping", ZMQ_PAIR);
    zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};

    while (true) {
      std::unique_lock<std::mutex> lock(stop_reply_mutex);

      spdlog::debug("Wait with reply for test");
      // We send every given seconds a ping or we stop the hearbeat
      if (stop_reply.wait_for(lock, timeout) == std::cv_status::no_timeout) {
        spdlog::info("Terminate reply. Received the stop condition.");
        break;  // Exit the loop because stop_reply condition is set
      }

      zmq::poll(&items[0], 1, 1);  // 1 second timeout

      if (items[0].revents & ZMQ_POLLIN) {
        auto msg = zmqutils::RecvString(socket);
        spdlog::info("Got ping message: {}", msg);

        spdlog::info("Send pong messge");
        auto pong_socket = zmqutils::Connect(barbarossa::gInProcContext,
                                             "inproc://pong", ZMQ_PAIR);
        zmqutils::SendString(pong_socket, "PONG");
        timeout++;
      }
    }
  });


  heartbeat.join();
  reply.join();*/

  //
  // END OF IMPORTANT CODE FOR CONTROL CHANNEL
  //

  // t.join();

  // Start thread
  // std::thread t1(&ListenEvents, std::ref(barbarossa::gInProcContext));
  // std::thread t2(&SendEvents, std::ref(barbarossa::gInProcContext));

  // auto quit_socket = zmqutils::Connect(context, "inproc://quit", ZMQ_PAIR);
  // zmqutils::SendString(quit_socket, "QUIT");

  // t2.join();
  // t1.join();


  using namespace barbarossa::controlchannel::v1;
  WebsocketEndpoint endpoint(argv[1]);
  ControlChannel<WebsocketEndpoint> control_channel(endpoint, argv[2]);
  control_channel.Run();

  return 0;
}
