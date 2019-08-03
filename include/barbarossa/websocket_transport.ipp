// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#include <exception>

#include "barbarossa/errors.hpp"
#include "barbarossa/message.hpp"
#include "barbarossa/transport.hpp"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace barbarossa::controlchannel {

using json = nlohmann::json;

template <typename CONFIG>
inline WebSocketTransport<CONFIG>::WebSocketTransport(client_t& client,
                                                      const std::string& uri)
    : client_(client), uri_(uri), open_(false), done_(false) {
  // Moved all websocketpp initialization code to a seperate method.
  spdlog::debug("transport: instance is created");
  StartClient();
}

template <typename CONFIG>
inline WebSocketTransport<CONFIG>::~WebSocketTransport() {
  // If websocket client isn't closed by Disconnect() we received an opcode 8.
  // The server closed our connection and that's why we're waiting until the
  // thread is done before we release the instance.
  if (!done_) {
    thread_->join();
  }

  spdlog::debug("transport: instance is destroyed");
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Connect() {
  spdlog::debug("transport: connecting");
  if (open_) {
    throw std::logic_error(ErrorMessages::kTransportAlreadyConnected);
  }

  websocketpp::lib::error_code ec;
  // typename client_t::connection_ptr connection_ptr_t;
  auto conn = client_.get_connection(uri_, ec);
  if (ec) {
    throw websocketpp::lib::system_error(ec.value(), ec.category(), "connect");
  }

  // Grab the handle for this connection
  hdl_ = conn->get_handle();

  // Queue the connection
  client_.connect(conn);

  // Wait for connected_ promise
  auto connected_future = connected_.get_future();

  spdlog::debug(
      "transport: connect is waiting for websocket client on open callback");
  connected_future.get();  // This future can throw an exception
  spdlog::debug("transport: connected");
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Disconnect() {
  spdlog::debug("transport: disconnecting");
  if (!open_) {
    throw std::logic_error(ErrorMessages::kTransportAlreadyDisconnected);
  }

  StopClient("disconnect");  // TODO(DGL) This is regular shutdown.
  spdlog::debug("transport: disconnected");
}

template <typename CONFIG>
inline bool WebSocketTransport<CONFIG>::IsConnected() const {
  return open_ && !done_;
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::SendMessage(Message&& message) {
  auto j = json::array();
  for (std::size_t i = 0; i < message.Size(); i++) {
    j.push_back(message.Field(i));
  }
  spdlog::debug("transpport: sending message: {}", j.dump());

  websocketpp::lib::error_code ec;
  client_.send(hdl_, j.dump(), websocketpp::frame::opcode::text, ec);
  if (ec) {
    spdlog::error("websocket_transport: error sending message: {}",
                  ec.message());
    throw NetworkError(ec.message());
  }
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Attach(
    const std::shared_ptr<TransportHandler>& handler) {
  spdlog::debug("transport: attaching transport handler");

  if (handler_) {
    throw std::logic_error(ErrorMessages::kTransportHandlerAlreadyAttached);
  }

  handler_ = handler;
  handler_->OnAttach(this->shared_from_this());
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Detach() {
  spdlog::debug("transport: detaching transport handler");
  if (!handler_) {
    throw std::logic_error(ErrorMessages::kTransportHandlerAlreadyDetached);
  }

  // TODO(DGL) Useful reason for detaching a handler???
  handler_->OnDetach("goodbye");
  handler_.reset();
}

template <typename CONFIG>
inline bool WebSocketTransport<CONFIG>::HasHandler() const {
  return handler_ != nullptr;
}

// StartClient is called by the ctor. It inits websocketpp, attaches the
// callbacks and starts the thread.
template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::StartClient() {
  spdlog::debug("transport: websocket client is starting");

  client_.set_access_channels(websocketpp::log::alevel::connect);
  client_.set_access_channels(websocketpp::log::alevel::disconnect);
  client_.set_access_channels(websocketpp::log::alevel::app);
  // client_.clear_access_channels(websocketpp::log::alevel::all);

  using websocketpp::lib::bind;
  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::placeholders::_2;
  client_.set_open_handler(
      bind(&WebSocketTransport<CONFIG>::OnClientOpen, this, _1));
  client_.set_close_handler(
      bind(&WebSocketTransport<CONFIG>::OnClientClose, this, _1));
  client_.set_fail_handler(
      bind(&WebSocketTransport<CONFIG>::OnClientFail, this, _1));
  client_.set_message_handler(
      bind(&WebSocketTransport<CONFIG>::OnClientMessage, this, _1, _2));


  // On websocket is open handler sets a successfull connected promise to tell
  // our Connect method that our connection is established.
  /*client_.set_open_handler([this](websocketpp::connection_hdl) {
    spdlog::debug("transport: websocket client on open is called");
    scoped_lock_t guard(lock_);
    open_ = true;
    connected_.set_value();  // Release successfully the Connect() method
  });*/

  // On websocket is close handler sets our transport state to disconnected.
  /*client_.set_close_handler([this](websocketpp::connection_hdl) {
    spdlog::debug("transport: websocket client on close is called");
    scoped_lock_t guard(lock_);
    done_ = true;
  });*/

  // On websocket is fail handler sets an exception to our connected promise to
  // tell our Connect method that the connection failed.
  /*client_.set_fail_handler([this](websocketpp::connection_hdl) {
    spdlog::debug("transport: websocket client on fail is called");
    if (!open_) {
      // Release the Connect() method with a failure
      connected_.set_exception(std::make_exception_ptr(
          NetworkError(ErrorMessages::kTransportConnectionFailed)));
    }
    scoped_lock_t guard(lock_);
    done_ = true;
  });*/

  /*client_.set_message_handler(
      [this](websocketpp::connection_hdl, message_ptr_t msg) {
        spdlog::debug("transport: websocket client on message is called");
        if (msg->get_opcode() != websocketpp::frame::opcode::text) {
          spdlog::error("unsupported binary data");
          // TODO(DGL) What should we do now? Raise ProtocolError?
          return;
        }
        RecvMessage(msg->get_payload());
      });*/

  client_.start_perpetual();
  thread_.reset(new websocketpp::lib::thread(&client_t::run, &client_));

  spdlog::debug("transport: websocket client is started");
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::OnClientOpen(
    websocketpp::connection_hdl) {
  spdlog::debug("transport: websocket client on open is called");
  scoped_lock_t guard(lock_);
  open_ = true;
  connected_.set_value();  // Release successfully the Connect() method
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::OnClientClose(
    websocketpp::connection_hdl) {
  spdlog::debug("transport: websocket client on close is called");
  scoped_lock_t guard(lock_);
  done_ = true;
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::OnClientFail(
    websocketpp::connection_hdl) {
  spdlog::debug("transport: websocket client on fail is called");

  // If failed callback is called but open callback wasn't called (open ==
  // false) than we have a connection problem.
  if (!open_) {
    connected_.set_exception(std::make_exception_ptr(
        NetworkError(ErrorMessages::kTransportConnectionFailed)));
  }

  spdlog::error("transport: websocket client failed");

  scoped_lock_t guard(lock_);
  done_ = true;
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::OnClientMessage(
    websocketpp::connection_hdl, message_ptr_t msg) {
  spdlog::debug("transport: websocket client on message is called");
  if (msg->get_opcode() != websocketpp::frame::opcode::text) {
    spdlog::error("unsupported binary data");
    // TODO(DGL) What should we do now? Raise ProtocolError?
    return;
  }
  RecvMessage(msg->get_payload());
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::StopClient(
    const std::string& reason) noexcept {
  spdlog::debug("transport: websocket client is stopping");
  // Leave if we closed already, e.g. with Disconnect()
  if (done_) {
    return;
  }

  // In StartEndpoint we executed start_perpetual and now we're doing the
  // opposit.
  client_.stop_perpetual();

  // Close the websocket connection
  client_.close(hdl_, websocketpp::close::status::normal, reason);

  // We're waiting until the websocketpp thread finishes it's work. Sample code
  // runs the join in the destructor. I don't like that because you don't know
  // why and where your thread is blocking.
  thread_->join();

  open_ = false;

  spdlog::debug("transport: websocket client is stopped");
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::RecvMessage(
    const std::string& payload) {
  if (!handler_) {
    spdlog::error("no transport handler attached");
    // TODO(DGL) What should we do now? Raise an error?
    return;
  }

  spdlog::debug("transport: message received: {}", payload);

  // Parse the message.
  // TODO(DGL) We should create a Codec class
  auto j = json::parse(payload);
  if (!j.is_array()) {
    spdlog::error("protocol error: invalid message received.");
    // TODO(DGL) We should raise an protocol error?
    return;
  }

  Message message(j.size());
  std::size_t i = 0;
  for (auto& field : j) {
    message.SetField(i++, field);
  }

  handler_->OnMessage(std::move(message));
}

}  // namespace barbarossa::controlchannel