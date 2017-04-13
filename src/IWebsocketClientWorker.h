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

#ifndef WSCPP_IWEBSOCKETCLIENTWORKER_H
#define WSCPP_IWEBSOCKETCLIENTWORKER_H

#include <cstdint>

namespace wscpp
{

/**
 * This class provides a type-erased interface for WebsocketClientWorker and
 * WebsocketClientWorkerTls.
 */
class IWebsocketClientWorker
{
public:
  virtual ~IWebsocketClientWorker() = default;

  /**
   * @brief transmit text message via websockets
   * @param data message to be sent, will be interpreted as text
   * @param size size of message to be sent
   */
  virtual void sendTextMessage(const char* data, std::size_t size) = 0;

  /**
   * @brief transmit binary message via websockets
   * @param data binary message to be sent
   * @param size size of message to be sent
   */
  virtual void sendBinaryMessage(const char* data, std::size_t size) = 0;

  /**
   * @brief closes the underlying websocket connection
   */
  virtual void close() = 0;
};

} // namespace wscpp

#endif // WSCPP_IWEBSOCKETCLIENTWORKER_H
