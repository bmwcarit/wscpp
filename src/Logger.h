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

#ifndef WSCPP_LOGGER_H
#define WSCPP_LOGGER_H

#include <functional>
#include <string>

#include <websocketpp/logger/levels.hpp>

namespace wscpp
{
class BaseLogger
{
public:
  using Level = websocketpp::log::level;
  using ChannelHint = websocketpp::log::channel_type_hint::value;
  using LogCallback = std::function<void(Level, const std::string&)>;
  void set_channels(Level) {}
  void clear_channels(Level) {}
};

// log everything
class Logger : public BaseLogger
{
public:
  explicit Logger(ChannelHint) {}
  Logger(Level, ChannelHint) {}

  void write(Level l, const std::string& msg)
  {
    if (callback) {
      callback(l, msg);
    }
  }

  void write(Level l, char const* msg)
  {
    if (callback) {
      callback(l, std::string(msg));
    }
  }

  bool static_test(Level) const { return true; }
  bool dynamic_test(Level) { return true; }
  void setCallback(LogCallback callback) { this->callback = std::move(callback); }

private:
  LogCallback callback;
};

// log nothing
class NullLogger : public BaseLogger
{
public:
  explicit NullLogger(ChannelHint) {}
  NullLogger(Level, ChannelHint) {}
  void write(Level, const std::string&) {}
  void write(Level, char const*) {}
  bool static_test(Level) const { return false; }
  bool dynamic_test(Level) { return false; }
  void setCallback(LogCallback) {}
};
} // namespace wscpp

#endif // WSCPP_LOGGER_H
