// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#include <condition_variable>
#include <csignal>
#include <future>
#include <iostream>
#include <string>

#define ASIO_STANDALONE
#include "asio.hpp"
#include "barbarossa/session.hpp"
#include "barbarossa/transport.hpp"
#include "barbarossa/transport_handler.hpp"
#include "barbarossa/websocket_transport.hpp"
#include "spdlog/spdlog.h"
#include "websocketpp/config/asio_client.hpp"

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
  spdlog::info("endpoint created");

  // Our websocket runs over TLS and therefore it's required to setup a TLS init
  // handler for the websocketpp implementation.
  client.set_tls_init_handler([](websocketpp::connection_hdl) {
    spdlog::debug("endpoint tls init handler");

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
  spdlog::info("endpoint ready");

  // Create our control channel transport layer.
  auto transport = std::make_shared<
      controlchannel::WebSocketTransport<websocketpp::config::asio_tls_client>>(
      client, argv[1]);
  spdlog::info("transport ready");

  // Create our control channel session layer.
  auto session = std::make_shared<controlchannel::Session>(io_context);

  // Wire signal handler
  asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto) {
    spdlog::debug("signal received");
    session->Leave();  // TODO(DGL) Add leave message?
    io_context.stop();
  });

  // Wire transport and session layer together and connect to the server.
  transport->Attach(
      std::static_pointer_cast<controlchannel::TransportHandler>(session));
  transport->Connect();
  spdlog::info("transport connected");

  session->Start();
  spdlog::info("session started");

  session->Join(argv[2]);
  spdlog::info("session joined");

  io_context.run();

  return 0;
}
