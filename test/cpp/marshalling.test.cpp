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

#include <cstdint>
#include <memory>
#include <string>

#include <nan.h>

#include "marshalling.h"

template <typename T>
auto ToLocalCheckedWrapper(v8::MaybeLocal<T> value)
{
  return value.ToLocalChecked();
}

template <typename T>
auto ToLocalCheckedWrapper(T value)
{
  return value;
}

template <typename T>
auto NewWrapper(T scalar)
{
  return Nan::New(scalar);
}

template <typename T>
auto NewWrapper(std::vector<T> vec)
{
  v8::Local<v8::Array> array = v8::Local<v8::Array>(Nan::New<v8::Array>());
  std::uint32_t index = 0;
  for (auto&& e : vec) {
    auto arrrayElement = Nan::New(e);
    Nan::Set(array, Nan::New(index++), ToLocalCheckedWrapper(arrrayElement));
  }
  return array;
}

template <typename T>
void getMemberValue(Nan::NAN_METHOD_ARGS_TYPE info)
{
  using wscpp::marshalling::convertFromV8;
  using wscpp::marshalling::getOptionalMemberValue;

  v8::Local<v8::Object> context = info[0].As<v8::Object>();

  boost::optional<T> value;
  convertFromV8(getOptionalMemberValue(context, "value"), value);
  if (!value.is_initialized()) {
    return Nan::ThrowError("'value' is not set");
  }
  info.GetReturnValue().Set(ToLocalCheckedWrapper(NewWrapper(*value)));
}

NAN_METHOD(getString) { getMemberValue<std::string>(info); }
NAN_METHOD(getInteger) { getMemberValue<std::int32_t>(info); }
NAN_METHOD(getDouble) { getMemberValue<double>(info); }
NAN_METHOD(getBool) { getMemberValue<bool>(info); }

NAN_METHOD(getStringVector) { getMemberValue<std::vector<std::string>>(info); }
NAN_METHOD(getIntegerVector) { getMemberValue<std::vector<std::int32_t>>(info); }
NAN_METHOD(getDoubleVector) { getMemberValue<std::vector<double>>(info); }
NAN_METHOD(getBoolVector) { getMemberValue<std::vector<bool>>(info); }

NAN_METHOD(invokeCallback)
{
  using wscpp::marshalling::convertFromV8;
  using wscpp::marshalling::getOptionalMemberValue;

  v8::Local<v8::Object> context = info[0].As<v8::Object>();
  std::unique_ptr<Nan::Callback> callback;
  convertFromV8(getOptionalMemberValue(context, "callback"), callback);

  if (!callback) {
    return Nan::ThrowError("'callback' is not set");
  }

  callback->Call(0, nullptr);
}
NAN_MODULE_INIT(Init)
{
  using namespace Nan;
  Set(target, New<v8::String>("getString").ToLocalChecked(),
      New<v8::FunctionTemplate>(getString)->GetFunction());
  Set(target, New<v8::String>("getInteger").ToLocalChecked(),
      New<v8::FunctionTemplate>(getInteger)->GetFunction());
  Set(target, New<v8::String>("getDouble").ToLocalChecked(),
      New<v8::FunctionTemplate>(getDouble)->GetFunction());
  Set(target, New<v8::String>("getBool").ToLocalChecked(),
      New<v8::FunctionTemplate>(getBool)->GetFunction());

  Set(target, New<v8::String>("getStringVector").ToLocalChecked(),
      New<v8::FunctionTemplate>(getStringVector)->GetFunction());
  Set(target, New<v8::String>("getIntegerVector").ToLocalChecked(),
      New<v8::FunctionTemplate>(getIntegerVector)->GetFunction());
  Set(target, New<v8::String>("getDoubleVector").ToLocalChecked(),
      New<v8::FunctionTemplate>(getDoubleVector)->GetFunction());
  Set(target, New<v8::String>("getBoolVector").ToLocalChecked(),
      New<v8::FunctionTemplate>(getBoolVector)->GetFunction());

  Set(target, New<v8::String>("invokeCallback").ToLocalChecked(),
      New<v8::FunctionTemplate>(invokeCallback)->GetFunction());
}

NODE_MODULE(marshallingTest, Init)
