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

#ifndef WSCPP_WEBSOCKETCLIENTWORKER_H
#define WSCPP_WEBSOCKETCLIENTWORKER_H

#include "ConfigWithLogger.h"
#include "GenericWebsocketClientWorker.h"

namespace wscpp
{

template <typename Logger>
using PlainConfigWithLogger =
    ConfigWithLogger<Logger, websocketpp::transport::asio::basic_socket::endpoint>;

template <typename Logger>
class WebsocketClientWorker : public GenericWebsocketClientWorker<PlainConfigWithLogger<Logger>>
{
  using Base = GenericWebsocketClientWorker<PlainConfigWithLogger<Logger>>;

public:
  WebsocketClientWorker(std::unique_ptr<Parameters> parameters) : Base(std::move(parameters))
  {
    this->start();
  }
};

} // namespace wscpp

#endif // WSCPP_IWEBSOCKETCLIENTWORKER_H
