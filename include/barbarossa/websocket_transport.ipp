// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#include <exception>

#include "barbarossa/message.hpp"
#include "barbarossa/transport.hpp"

namespace barbarossa::controlchannel {

inline WebSocketTransport::WebSocketTransport(const std::string& uri)
    : uri_(uri), open_(false), done_(false) {
  // On websocket is open handler sets a successfull connected promise to tell
  // our Connect method that our connection is established.
  endpoint_.set_open_handler([&](websocketpp::connection_hdl) {
    scoped_lock guard(lock_);
    open_ = true;
    connected_.set_value();  // Release successfully the Connect() method
  });

  // On websocket is close handler sets our transport state to disconnected.
  endpoint_.set_close_handler([&](websocketpp::connection_hdl) {
    scoped_lock guard(lock_);
    done_ = true;
  });

  // On websocket is fail handler sets an exception to our connected promise to
  // tell our Connect method that the connection failed.
  endpoint_.set_fail_handler([&](websocketpp::connection_hdl) {
    if (!open_) {
      // Release the Connect() method with a failure
      connected_.set_exception(std::make_exception_ptr(
          NetworkError("network transport failed to connect")));
    }
    scoped_lock guard(lock_);
    done_ = true;
  });
}

inline void WebSocketTransport::Connect() {
  if (open_) {
    throw NetworkError("network transport already connected");
  }

  websocketpp::lib::error_code ec;
  client::connection_ptr con = endpoint_.get_connection(uri_, ec);
  if (ec) {
    throw websocketpp::lib::system_error(ec.value(), ec.category(), "connect");
  }

  // Grab the handle for this connection
  hdl_ = con->get_handle();

  // Queue the connection
  endpoint_.connect(con);

  // Wait for connected_ promise
  auto connected_future = connected_.get_future();
  connected_future.get();  // This future can throw an exception
}

inline void WebSocketTransport::Disconnect() {
  if (open_) {
    throw NetworkError("network transport already disconnected");
  }
  endpoint_.close(hdl_, websocketpp::close::status::normal, "disconnect");
}

inline bool WebSocketTransport::IsConnected() const { return open_ && !done_; }

inline void WebSocketTransport::SendMessage(Message&& message) {}

inline void WebSocketTransport::Attach(
    const std::shared_ptr<TransportHandler>& handler) {
  if (handler_) {
    throw std::logic_error("handler already attached");
  }

  handler_ = handler;
  handler_->OnAttach(shared_from_this());
}

inline void WebSocketTransport::Detach() {
  if (handler_) {
    throw std::logic_error("no handler attached");
  }

  // TODO(DGL) Useful reason for detaching a handler???
  handler_->OnDetach("goodbye");
  handler_.reset();
}

inline bool WebSocketTransport::HasHandler() const {
  return handler_ != nullptr;
}

}  // namespace barbarossa::controlchannel