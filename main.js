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

'use strict';

function noop() {}

var NativeWebSocket = require('bindings')('wscpp-client.node');

class WebSocket {
  constructor(serverUri, options) {
    if (typeof serverUri !== 'string') {
      throw 'serverUri must be a string';
    }
    if (typeof options !== 'undefined') {
      if (options === null || typeof options !== 'object') {
        throw 'options must be an object';
      }
      if (options.ca !== 'undefined' && !(options.ca instanceof Array)) {
        options.ca = [options.ca];
      }
    }
    this.onOpenCallbackInternal = noop;
    this.onMessageCallbackInternal = noop;
    this.onCloseCallbackInternal = noop;
    this.onErrorCallbackInternal = noop;

    this.readyState = WebSocket.CONNECTING;
    this.nativeHandle = new NativeWebSocket.WebsocketClientWorker(serverUri, this, options || {});
  }

  set onopen(f) {
    this.onOpenCallbackInternal = f ? f : noop;
  }

  set onmessage(f) {
    this.onMessageCallbackInternal = f ? f : noop;
  }

  set onclose(f) {
    this.onCloseCallbackInternal = f ? f : noop;
  }

  set onerror(f) {
    this.onErrorCallbackInternal = f ? f : noop;
  }

  /**
   * @todo in order to fully comply with "ws" API, we would need to support
   * the "code" and "data" arguments
   */
  close(/*code, data*/) {
    this.nativeHandle.close();
    this.nativeHandle = null;
    this.readyState = WebSocket.CLOSING;
  }

  onOpenCallback() {
    this.readyState = WebSocket.OPEN;
    this.onOpenCallbackInternal();
  }

  onMessageCallback(msg) {
    this.onMessageCallbackInternal({'data': msg});
  }

  onCloseCallback() {
    this.readyState = WebSocket.CLOSED;
    this.onCloseCallbackInternal();
  }

  onErrorCallback() {
    this.onErrorCallbackInternal();
  }

  /**
   * @todo in order to fully comply with "ws" API, we would need to support
   * callbacks
   */
  send(message) {
    if (typeof message === 'number') {
      message = message.toString();
    }

    const isBinary = typeof message !== 'string';

    if (!Buffer.isBuffer(message)) {
      message = Buffer.from(message);
    }

    this.nativeHandle.send(message, isBinary);
  }
}

WebSocket.CONNECTING = 0;
WebSocket.OPEN = 1;
WebSocket.CLOSING = 2;
WebSocket.CLOSED = 3;
module.exports = WebSocket;
