#include "barbarossa/websocket_endpoint.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#define ASIO_STANDALONE

#include "spdlog/spdlog.h"
#include "websocketpp/common/memory.hpp"
#include "websocketpp/common/thread.hpp"

namespace barbarossa::controlchannel::v1 {

WebsocketEndpoint::WebsocketEndpoint(ControlChannel& control_channel)
    : initialized_(false),
      connected_(false),
      control_channel_(control_channel) {
  endpoint_.clear_access_channels(websocketpp::log::alevel::all);
  endpoint_.set_access_channels(websocketpp::log::alevel::connect);
  endpoint_.set_access_channels(websocketpp::log::alevel::disconnect);
  endpoint_.set_access_channels(websocketpp::log::alevel::app);
}

bool WebsocketEndpoint::Connect(const std::string& uri) {
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

  endpoint_.start_perpetual();
  thread_.reset(new websocketpp::lib::thread(&client::run, &endpoint_));

  initialized_ = true;

  // Initiate connection
  websocketpp::lib::error_code ec;
  client::connection_ptr con = endpoint_.get_connection(uri, ec);
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

bool WebsocketEndpoint::Send(const std::string& data) {
  if (!initialized_ || !connected_) {
    // TODO(DGL) Implement return error code!
    return false;
  }

  websocketpp::lib::error_code ec;
  endpoint_.send(hdl_, data, websocketpp::frame::opcode::text, ec);
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
  control_channel_.RaiseEvent(kControlChannelEventOnOpen);
}

void WebsocketEndpoint::OnClose(websocketpp::connection_hdl) {
  spdlog::debug("websocket_endpoint: Event on open");
  scoped_lock guard(lock_);
  connected_ = false;
  control_channel_.RaiseEvent(kControlChannelEventOnClose);
}

void WebsocketEndpoint::OnFail(websocketpp::connection_hdl) {
  spdlog::debug("websocket_endpoint: Event on open");
  scoped_lock guard(lock_);
  connected_ = false;
  control_channel_.RaiseEvent(kControlChannelEventOnFail);
}

void WebsocketEndpoint::OnMessage(websocketpp::connection_hdl,
                                  message_ptr msg) {
  spdlog::debug("websocket_endpoint: Event on open");
  control_channel_.RaiseEvent(kControlChannelEventOnMessage,
                              msg->get_payload());
}

}  // namespace barbarossa::controlchannel::v1