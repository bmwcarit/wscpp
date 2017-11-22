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

#ifndef WSCPP_CONFIGWITHLOGGER_H
#define WSCPP_CONFIGWITHLOGGER_H

#include <websocketpp/config/core_client.hpp>
#include <websocketpp/transport/asio/endpoint.hpp>

template <typename Logger, typename Socket>
struct ConfigWithLogger : public websocketpp::config::core_client {

  using alog_type = Logger;
  using elog_type = Logger;

  struct transport_config : public core_client::transport_config {
    using alog_type = Logger;
    using elog_type = Logger;
    using socket_type = Socket;
  };

  using transport_type = websocketpp::transport::asio::endpoint<transport_config>;
};

#endif // WSCPP_CONFIGWITHLOGGER_H
