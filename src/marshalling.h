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

#ifndef WSCPP_MARSHALLING_H
#define WSCPP_MARSHALLING_H

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/optional.hpp>
#include <nan.h>

namespace wscpp
{
namespace marshalling
{
using OptionalV8Value = boost::optional<v8::Local<v8::Value>>;

OptionalV8Value maybeToOptionalValue(Nan::MaybeLocal<v8::Value> maybeValue)
{
  OptionalV8Value optionalV8Value;
  if (!maybeValue.IsEmpty()) {
    v8::Local<v8::Value> value = maybeValue.ToLocalChecked();
    if (!value->IsUndefined()) {
      optionalV8Value = value;
    }
  }
  return optionalV8Value;
}

OptionalV8Value getOptionalMemberValue(v8::Local<v8::Object> context, const char* key)
{
  Nan::MaybeLocal<v8::Value> maybeValue = Nan::Get(context, Nan::New(key).ToLocalChecked());
  return maybeToOptionalValue(maybeValue);
}

void convertFromV8(OptionalV8Value optionalValue, std::unique_ptr<Nan::Callback>& callback)
{
  if (optionalValue) {
    callback = std::make_unique<Nan::Callback>(optionalValue->As<v8::Function>());
  }
}

void convertFromV8(OptionalV8Value optionalValue, bool& value)
{
  if (optionalValue) {
    value = optionalValue.get()->BooleanValue();
  }
}

void convertFromV8(OptionalV8Value optionalValue, std::string& value)
{
  if (optionalValue) {
    v8::String::Utf8Value stringValue(optionalValue.get()->ToString());
    value = *stringValue;
  }
}

template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value> convertFromV8(OptionalV8Value optionalValue,
                                                             T& value)
{
  if (optionalValue) {
    value = optionalValue.get()->NumberValue();
  }
}

template <typename T>
void convertFromV8(OptionalV8Value optionalValue, std::vector<T>& vector)
{
  if (optionalValue) {
    v8::Local<v8::Array> array = optionalValue->As<v8::Array>();
    for (std::size_t i = 0; i < array->Length(); ++i) {
      T value;
      convertFromV8(maybeToOptionalValue(array->Get(i)), value);
      vector.push_back(std::move(value));
    }
  }
}

template <typename T>
void convertFromV8(OptionalV8Value optionalV8Value, boost::optional<T>& optionalValue)
{
  if (optionalV8Value) {
    T value;
    convertFromV8(optionalV8Value, value);
    optionalValue = std::move(value);
  }
}

} // namespace marshalling
} // namespace wscpp

#endif // WSCPP_MARSHALLING_H
