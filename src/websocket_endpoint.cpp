// Copyright (c) 2018 by nsyszr.io.
// Author: dgl

#include "barbarossa/websocket_endpoint.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#define ASIO_STANDALONE

#include "asio.hpp"
#include "asio/ssl.hpp"
#include "spdlog/spdlog.h"
#include "websocketpp/common/memory.hpp"
#include "websocketpp/common/thread.hpp"

namespace barbarossa::controlchannel::v1 {

WebsocketEndpoint::WebsocketEndpoint(const std::string& uri)
    : uri_(uri), initialized_(false), connected_(false) {
  endpoint_.clear_access_channels(websocketpp::log::alevel::all);
  endpoint_.set_access_channels(websocketpp::log::alevel::connect);
  endpoint_.set_access_channels(websocketpp::log::alevel::disconnect);
  endpoint_.set_access_channels(websocketpp::log::alevel::app);
}

bool WebsocketEndpoint::Connect() {
  // TODO(DGL) Catch the exception of init_asio and return error code instead!
  endpoint_.init_asio();

  // Bind the handlers we are using
  using websocketpp::lib::bind;
  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::placeholders::_2;
  endpoint_.set_open_handler(bind(&WebsocketEndpoint::OnOpen, this, _1));
  endpoint_.set_close_handler(bind(&WebsocketEndpoint::OnClose, this, _1));
  endpoint_.set_fail_handler(bind(&WebsocketEndpoint::OnFail, this, _1));
  endpoint_.set_message_handler(
      bind(&WebsocketEndpoint::OnMessage, this, _1, _2));

  // Temporary code
  endpoint_.set_tls_init_handler(bind(&WebsocketEndpoint::OnTLSInit, this));

  endpoint_.start_perpetual();
  thread_.reset(new websocketpp::lib::thread(&client::run, &endpoint_));

  initialized_ = true;

  // Initiate connection
  websocketpp::lib::error_code ec;
  client::connection_ptr con = endpoint_.get_connection(uri_, ec);
  if (ec) {
    // TODO(DGL) Implement return error code!
    spdlog::error("websocket_endpoint: error connecting: {}", ec.message());
    return false;
  }

  // Grab the handle for this connection
  hdl_ = con->get_handle();

  // Queue the connection
  endpoint_.connect(con);

  return true;
}

WebsocketEndpoint::~WebsocketEndpoint() {
  if (!initialized_) {
    // There's nothing to do because we're not initialized
    return;
  }

  endpoint_.stop_perpetual();
  Close();
  thread_->join();
}

bool WebsocketEndpoint::Send(const std::string& payload) {
  if (!initialized_ || !connected_) {
    // TODO(DGL) Implement return error code!
    return false;
  }

  websocketpp::lib::error_code ec;
  endpoint_.send(hdl_, payload, websocketpp::frame::opcode::text, ec);
  if (ec) {
    // TODO(DGL) Implement return error code!
    spdlog::debug("websocket_endpoint: error sending message: {}",
                  ec.message());
    return false;
  }

  return true;
}

bool WebsocketEndpoint::Close() {
  if (!initialized_) {
    // TODO(DGL) Implement return error code!
    return false;
  }
  if (!connected_) {
    return true;
  }

  websocketpp::lib::error_code ec;
  endpoint_.close(hdl_, websocketpp::close::status::going_away, "", ec);
  if (ec) {
    // TODO(DGL) Implement return error code!
    spdlog::debug("websocket_endpoint: error closing connection: {}",
                  ec.message());
    return false;
  }

  return true;
}

void WebsocketEndpoint::OnOpen(websocketpp::connection_hdl) {
  spdlog::debug("websocket_endpoint: Event on open");
  scoped_lock guard(lock_);
  connected_ = true;
  if (on_open_handler_) {
    on_open_handler_();
  }
}

void WebsocketEndpoint::OnClose(websocketpp::connection_hdl) {
  spdlog::debug("websocket_endpoint: Event on open");
  scoped_lock guard(lock_);
  connected_ = false;
  if (on_close_handler_) {
    on_close_handler_();
  }
}

void WebsocketEndpoint::OnFail(websocketpp::connection_hdl) {
  spdlog::debug("websocket_endpoint: Event on open");
  scoped_lock guard(lock_);
  connected_ = false;
  if (on_fail_handler_) {
    on_fail_handler_();
  }
}

void WebsocketEndpoint::OnMessage(websocketpp::connection_hdl,
                                  message_ptr msgp) {
  spdlog::debug("websocket_endpoint: Event on open");
  if (on_message_handler_) {
    on_message_handler_(msgp->get_payload());
  }
}

WebsocketEndpoint::context_ptr WebsocketEndpoint::OnTLSInit(
    /*const char* hostname, websocketpp::connection_hdl*/) {
  // establishes a SSL connection
  context_ptr ctx =
      std::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

  try {
    ctx->set_options(
        asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
        asio::ssl::context::no_sslv3 | asio::ssl::context::single_dh_use);
  } catch (std::exception& e) {
    std::cout << "Error in context pointer: " << e.what() << std::endl;
  }
  return ctx;
}

}  // namespace barbarossa::controlchannel::v1
