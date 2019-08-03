// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#include <condition_variable>
#include <csignal>
#include <future>
#include <iostream>
#include <string>

#include "barbarossa/session.hpp"
#include "barbarossa/transport.hpp"
#include "barbarossa/transport_handler.hpp"
#include "barbarossa/websocket_transport.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "websocketpp/config/asio_client.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
  // In implementation files the using namespace is allowed.
  using namespace barbarossa;  // NOLINT

  spdlog::set_level(spdlog::level::debug);

  if (argc <= 2) {
    std::cout << "Please provide the device control URI and device realm"
              << std::endl;
    return 1;
  }

  typedef websocketpp::client<websocketpp::config::asio_tls_client> client_t;
  typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>
      context_ptr;

  asio::io_context io_context(1);

  // Create a websocketpp client and wire it with our asio I/O service.
  client_t client;
  client.init_asio(&io_context);

  // Our websocket runs over TLS and therefore it's required to setup a TLS init
  // handler for the websocketpp implementation.
  client.set_tls_init_handler([](websocketpp::connection_hdl) {
    spdlog::debug("main: websocket client tls init handler called");
    context_ptr ctx =
        std::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
      ctx->set_options(asio::ssl::context::default_workarounds |
                       asio::ssl::context::no_sslv2 |
                       asio::ssl::context::no_sslv3 |
                       asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
      std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return ctx;
  });

  // Create our control channel transport layer. It has to be a shared pointer
  // because we pass transport as shared pointer to OnAttach method of session.
  auto transport = std::make_shared<
      controlchannel::WebSocketTransport<websocketpp::config::asio_tls_client>>(
      client, argv[1]);
  spdlog::debug("main: transport is created");

  // Create our control channel session layer.
  auto session = std::make_shared<controlchannel::Session>(io_context);
  spdlog::debug("main: session is created");

  // Register operations
  session->RegisterOperation("say_hello", [](json&& arguments) {
    auto result = json::object();

    if (arguments.find("name") != arguments.end()) {
      auto name = arguments["name"].get<std::string>();
      result["say_hello"] = "Hello " + name + "!";
    } else {
      result["say_hello"] = "Hello World!";
    }

    return result;
  });
  spdlog::debug("main: session operations are registered");

  // Wire signal handler
  asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto) {
    spdlog::debug("signal received");
    session->Leave();  // TODO(DGL) Add leave message?
    io_context.stop();
  });
  spdlog::debug("main: signal handler is created");

  // Wire transport and session layer together
  transport->Attach(
      std::static_pointer_cast<controlchannel::TransportHandler>(session));
  spdlog::debug("main: transport is attached to session");

  try {
    // Connect the control channel server
    transport->Connect();
    spdlog::debug("main: transport is connected");

    // Start the session by joining it
    session->Join(argv[2]);
    spdlog::debug("main: session is joined");

    // Blocking
    session->Listen();
  } catch (const controlchannel::AbortError& e) {
    spdlog::error("controlchannel aborted");
    io_context.stop();
  } catch (const controlchannel::TimeoutError& e) {
    spdlog::error("controlchannel timeout");
    // TODO(DGL) Restart?
    io_context.stop();
  } catch (const controlchannel::NetworkError& e) {
    spdlog::error("controlchannel network error");
    // TODO(DGL) Restart?
    io_context.stop();
  }

  // transport->Disconnect();
  // spdlog::debug("main: transport is disconnected");
  transport->Detach();
  spdlog::debug("main: transport is detached from session");

  // Start our ASIO io_context in background. The run method is confusing! Our
  // application is already running above. The run method ensures that the
  // asio context is properly cleaned up before leaving the app.
  io_context.run();
  // std::thread t([&io_context]() { io_context.run(); });
  // t.join();

  spdlog::debug("main: exit");

  return 0;
}
