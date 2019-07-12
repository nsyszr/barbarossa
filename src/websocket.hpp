#ifndef WEBSOCKET_HPP_
#define WEBSOCKET_HPP_

#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "protocol.hpp"

#define ASIO_STANDALONE

#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_no_tls_client.hpp"

#include "websocketpp/common/memory.hpp"
#include "websocketpp/common/thread.hpp"

using namespace iotcore::devicecontrol::v1;

enum ControlChannelStates {
  kControlChannelStateOpen = 0,
  kControlChannelStateConnected = 1,
  kControlChannelStateGreeted = 2,
  kControlChannelStateRegistered = 3,
  kControlChannelStateDisconnected = 4,
};

class WebsocketEndpoint {
 public:
  typedef websocketpp::client<websocketpp::config::asio_client> client;
  typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
  typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

  WebsocketEndpoint() : state_(kControlChannelStateOpen) {
    endpoint_.clear_access_channels(websocketpp::log::alevel::all);
    endpoint_.set_access_channels(websocketpp::log::alevel::connect);
    endpoint_.set_access_channels(websocketpp::log::alevel::disconnect);
    endpoint_.set_access_channels(websocketpp::log::alevel::app);

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
  }

  ~WebsocketEndpoint() {
    endpoint_.stop_perpetual();

    // Close the connection graceful if it's still open
    if (GetSafeState() != kControlChannelStateDisconnected) {
      std::cout << "> Closing connection " << std::endl;

      websocketpp::lib::error_code ec;
      endpoint_.close(hdl_, websocketpp::close::status::going_away, "", ec);
      if (ec) {
        std::cout << "> Error closing connection: " << ec.message()
                  << std::endl;
      }
    }

    thread_->join();
  }

  void Run(std::string const& uri) {
    websocketpp::lib::error_code ec;

    client::connection_ptr con = endpoint_.get_connection(uri, ec);
    if (ec) {
      std::cout << "> Connect initialization error: " << ec.message()
                << std::endl;
      // TODO(DGL) Should we throw an exception for better error handling?
      return;
    }

    // Grab the handle for this connection
    hdl_ = con->get_handle();

    // Queue the connection
    endpoint_.connect(con);

    // Run the handler and wait
    websocketpp::lib::thread handle_thread(&WebsocketEndpoint::Handler, this);
    handle_thread.join();

    return;
  }

  void Close(websocketpp::close::status::value code, std::string reason) {
    websocketpp::lib::error_code ec;

    endpoint_.close(hdl_, code, reason, ec);
    if (ec) {
      std::cout << "> Error initiating close: " << ec.message() << std::endl;
    }
  }

  void OnOpen(websocketpp::connection_hdl) {
    endpoint_.get_alog().write(
        websocketpp::log::alevel::app,
        "Connection opened, starting control channel client!");

    SetSafeState(kControlChannelStateConnected);
  }

  // The close handler will signal that we should stop sending telemetry
  void OnClose(websocketpp::connection_hdl) {
    endpoint_.get_alog().write(
        websocketpp::log::alevel::app,
        "Connection closed, stopping control channel client!");
    SetSafeState(kControlChannelStateDisconnected);
  }

  // The fail handler will signal that we should stop sending telemetry
  void OnFail(websocketpp::connection_hdl) {
    endpoint_.get_alog().write(
        websocketpp::log::alevel::app,
        "Connection failed, stopping control channel client!");
    SetSafeState(kControlChannelStateDisconnected);
  }

  void OnMessage(websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload() << std::endl;

    auto j = json::parse(msg->get_payload());

    switch (GetSafeState()) {
      case kControlChannelStateGreeted: {
        // We expect a hello or abort message now before we change the state
        auto welcome_msg = j.get<protocol::welcomemessage::WelcomeMessage>();
        SetSafeState(kControlChannelStateRegistered);
      } break;
    }

    /* websocketpp::lib::error_code ec;

    c->send(hdl, msg->get_payload(), msg->get_opcode(), ec);
    if (ec) {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    } */
  }

  void Handler() {
    int timeout = 0;
    bool done = false;
    websocketpp::lib::error_code ec;

    while (1) {
      // Processing
      switch (GetSafeState()) {
        case kControlChannelStateOpen: {
          // Do nothing, we're waiting another second until it's connected
        } break;
        case kControlChannelStateConnected: {
          json j = protocol::HelloMessage("barbarossa@test");
          endpoint_.send(hdl_, j.dump(), websocketpp::frame::opcode::text, ec);

          SetSafeState(kControlChannelStateGreeted);
        } break;
        case kControlChannelStateRegistered: {
          if (timeout == 30) {
            json j = protocol::PingMessage();
            endpoint_.send(hdl_, j.dump(), websocketpp::frame::opcode::text,
                           ec);
            timeout = 0;
          } else {
            timeout++;
          }
        } break;
        case kControlChannelStateDisconnected: {
          endpoint_.get_alog().write(
              websocketpp::log::alevel::app,
              "Control Channel was closed, leave the handler");
          done = true;
        }
      }

      // check for errors
      if (ec) {
        endpoint_.get_alog().write(websocketpp::log::alevel::app,
                                   "Send Error: " + ec.message());
        break;
      }

      // Leave the handler if we're done
      if (done) {
        break;
      }
      sleep(1);
    }
  }

 private:
  client endpoint_;
  websocketpp::lib::mutex lock_;
  websocketpp::connection_hdl hdl_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  ControlChannelStates state_;
  // bool open_;
  // bool done_;
  // bool registered_;

  ControlChannelStates GetSafeState() {
    scoped_lock guard(lock_);
    return state_;
  }

  void SetSafeState(ControlChannelStates state) {
    scoped_lock guard(lock_);
    std::cout << "State transition from " << state_ << " to " << state
              << std::endl;
    state_ = state;
  }
};

#endif  // WEBSOCKET_HPP_