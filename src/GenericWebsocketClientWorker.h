/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

#ifndef WSCPP_GENERICWEBSOCKETCLIENTWORKER_H
#define WSCPP_GENERICWEBSOCKETCLIENTWORKER_H

#include <memory>
#include <tuple> // for std::ignore

#include <nan.h>
#include <uv.h>

#include <concurrentqueue.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>

#include "IWebsocketClientWorker.h"
#include "Parameters.h"

namespace wscpp
{

/**
 * This class is modeled after NaN::AsyncWorker and
 * NaN::AsyncProgressWorkerBase.
 * It provides the runtime of the boost::asio::io_service event loop used for
 * websocket
 * communication and handles its interaction with the node event loop.
 */
template <typename Config>
class GenericWebsocketClientWorker : public IWebsocketClientWorker
{
private:
  enum class EventCallbackType { None, Open, Error, Close };
  using Endpoint = websocketpp::client<Config>;
  using MessagePtr = typename Config::message_type::ptr;

protected:
  using ConnectionHandle = websocketpp::connection_hdl;

public:
  GenericWebsocketClientWorker(std::unique_ptr<Parameters> parameters)
      : parameters(std::move(parameters)), endpoint(), work(), messageAsyncHandle(nullptr),
        eventAsyncHandle(nullptr), connectionHandle(), receivedMessageQueue(), eventQueue()
  {
    initLibUv();
    initWebsocketPp();
  }

  void initLibUv()
  {
    work.data = this;

    messageAsyncHandle = new uv_async_t();
    uv_async_init(uv_default_loop(), messageAsyncHandle, messageReceivedCallback);
    messageAsyncHandle->data = this;

    eventAsyncHandle = new uv_async_t();
    uv_async_init(uv_default_loop(), eventAsyncHandle, eventOccuredCallback);
    eventAsyncHandle->data = this;
  }

  ~GenericWebsocketClientWorker() override = default;

  void runEventLoop() { endpoint.run(); }

  static void asyncExecute(uv_work_t* req)
  {
    GenericWebsocketClientWorker* worker = static_cast<GenericWebsocketClientWorker*>(req->data);
    worker->runEventLoop();
  }

  static void asyncExecuteComplete(uv_work_t* req)
  {
    GenericWebsocketClientWorker* worker = static_cast<GenericWebsocketClientWorker*>(req->data);
    worker->handReceivedMessagesToNode();
    worker->handReceivedEventsToNode();
    uv_close(reinterpret_cast<uv_handle_t*>(worker->messageAsyncHandle), asyncCloseMessageHandle);
    uv_close(reinterpret_cast<uv_handle_t*>(worker->eventAsyncHandle), asyncCloseEventHandle);
  }

  void sendTextMessage(const char* data, std::size_t size) override
  {
    // TODO error handling? use overload with error code or let websocketpp throw an
    // exception?
    endpoint.send(connectionHandle, data, size, websocketpp::frame::opcode::text);
  }

  void sendBinaryMessage(const char* data, std::size_t size) override
  {
    endpoint.send(connectionHandle, data, size, websocketpp::frame::opcode::binary);
  }

  void close(std::uint16_t code, const std::string& reason) override
  {
    endpoint.stop_perpetual();
    websocketpp::lib::error_code ec;
    endpoint.close(connectionHandle, code, reason, ec);
    if (ec) {
      const std::string errorMessage = "could not close connection: " + ec.message();
      Nan::ThrowError(errorMessage.c_str());
    }
  }

  /**
   * @brief messageReceived
   * @param connectionHandle
   * @param msg
   */
  void messageReceived(ConnectionHandle connectionHandle, MessagePtr msg)
  {
    receivedMessageQueue.enqueue(std::move(msg));
    signalMessageReceivedToJS();
  }

  /**
   * will be called by node's main thread
   */
  static NAUV_WORK_CB(messageReceivedCallback)
  {
    auto worker = static_cast<GenericWebsocketClientWorker*>(async->data);
    worker->handReceivedMessagesToNode();
  }

  /**
   * will be called by node's main thread
   */
  static NAUV_WORK_CB(eventOccuredCallback)
  {
    auto worker = static_cast<GenericWebsocketClientWorker*>(async->data);
    worker->handReceivedEventsToNode();
  }

  inline static void asyncCloseMessageHandle(uv_handle_t* handle)
  {
    auto worker = static_cast<GenericWebsocketClientWorker*>(handle->data);
    delete reinterpret_cast<uv_async_t*>(handle);
    delete worker;
  }

  inline static void asyncCloseEventHandle(uv_handle_t* handle)
  {
    delete reinterpret_cast<uv_async_t*>(handle);
  }

  /**
   * @brief invokes the JS "onmessage" callback for each message which was
   * received via websockets
   * This method may only be called from node's main thread!
   */
  void handReceivedMessagesToNode()
  {
    Nan::HandleScope scope;
    auto context = Nan::New(parameters->thisContext);
    MessagePtr msg;
    while (receivedMessageQueue.try_dequeue(msg)) {
      const websocketpp::frame::opcode::value opcode = msg->get_opcode();
      if (opcode == websocketpp::frame::opcode::text) {
        v8::Local<v8::Value> argv[] = {Nan::New<v8::String>(msg->get_payload()).ToLocalChecked()};
        parameters->onMessageCallback->Call(context, 1, argv);
      } else if (opcode == websocketpp::frame::opcode::binary) {
        // TODO potentially one copy could be avoided here
        // this requires that websocketpp provides the possibility to move out
        // the payload from websocketpp::message_buffer
        // OR the shared_ptr is kept alive as long as the node::Buffer lives
        // char* rawData = const_cast<char*>(msg->get_payload().data());
        // v8::Local<v8::Value> argv[] = {
        //        Nan::NewBuffer(rawData,
        //        msg->get_payload().size()).ToLocalChecked()};
        v8::Local<v8::Value> argv[] = {
            Nan::CopyBuffer(msg->get_payload().data(), msg->get_payload().size()).ToLocalChecked()};
        parameters->onMessageCallback->Call(context, 1, argv);
      } else {
        // TODO error handling?
      }
    }
  }

  void handReceivedEventsToNode()
  {
    Nan::HandleScope scope;
    auto context = Nan::New(parameters->thisContext);
    EventCallbackType event;
    while (eventQueue.try_dequeue(event)) {
      switch (event) {
      case EventCallbackType::Open:
        parameters->onOpenCallback->Call(context, 0, nullptr);
        break;
      case EventCallbackType::Close:
        parameters->onCloseCallback->Call(context, 0, nullptr);
        break;
      case EventCallbackType::Error:
        parameters->onErrorCallback->Call(context, 0, nullptr);
        break;
      default:
        break;
      }
    }
  }

protected:
  /**
   * @brief start worker asynchronously
   *
   * This will run `asyncExecute` in libuv's thread pool, after that,
   *`asyncExecuteComplete` is called.
   */
  void start()
  {
    websocketpp::lib::error_code ec;
    typename Endpoint::connection_ptr con = endpoint.get_connection(parameters->serverUri, ec);
    if (ec) {
      throw std::runtime_error("could not create connection because: " + ec.message());
    }

    endpoint.connect(con);

    uv_queue_work(uv_default_loop(), &work, asyncExecute,
                  reinterpret_cast<uv_after_work_cb>(asyncExecuteComplete));
  }

  std::unique_ptr<Parameters> parameters;
  Endpoint endpoint;

private:
  void initWebsocketPp()
  {
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.init_asio();

    using namespace std::placeholders;
    endpoint.set_message_handler(
        bind(&GenericWebsocketClientWorker::messageReceived, this, _1, _2));
    endpoint.set_open_handler(bind(&GenericWebsocketClientWorker::connectionOpened, this, _1));
    endpoint.set_close_handler(bind(&GenericWebsocketClientWorker::connectionClosed, this, _1));
    endpoint.set_fail_handler(bind(&GenericWebsocketClientWorker::connectionFailed, this, _1));
  }

  void connectionOpened(ConnectionHandle connectionHandle)
  {
    this->connectionHandle = std::move(connectionHandle);
    signalEventOccurred(EventCallbackType::Open);
  }

  void connectionClosed(ConnectionHandle connectionHandle)
  {
    std::ignore = connectionHandle;
    signalEventOccurred(EventCallbackType::Close);
  }

  void connectionFailed(ConnectionHandle connectionHandle)
  {
    std::ignore = connectionHandle;
    // TODO add error description to event
    signalEventOccurred(EventCallbackType::Error);
  }

  /**
   * @brief notify node to call `messageReceivedCallback`
   */
  void signalMessageReceivedToJS() const { uv_async_send(messageAsyncHandle); }

  /**
   * @brief notify node to call `eventOccuredCallback`
   */
  void signalEventOccurred(EventCallbackType event)
  {
    eventQueue.enqueue(event);
    uv_async_send(eventAsyncHandle);
  }

  uv_work_t work;
  uv_async_t* messageAsyncHandle;
  uv_async_t* eventAsyncHandle;

  ConnectionHandle connectionHandle;

  moodycamel::ConcurrentQueue<MessagePtr> receivedMessageQueue;
  moodycamel::ConcurrentQueue<EventCallbackType> eventQueue;
};

} // namespace wscpp

#endif // WSCPP_GENERICWEBSOCKETCLIENTWORKER_H
