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

var NativeWebSocket;
try {
  NativeWebSocket = require('wscpp-client.node');
} catch (err) {
  NativeWebSocket = require('bindings')('wscpp-client.node');
};

class WebSocket {
  constructor(serverUri, options = {}) {
    if (typeof serverUri !== 'string') {
      throw new Error('serverUri must be a string');
    }
    if (options === null || typeof options !== 'object') {
      throw new Error('options must be an object');
    }
    if (options.ca !== undefined && !(options.ca instanceof Array)) {
      options.ca = [options.ca];
    }
    this.onOpenCallbackInternal = noop;
    this.onMessageCallbackInternal = noop;
    this.onCloseCallbackInternal = noop;
    this.onErrorCallbackInternal = noop;
    this.onLogCallbackInternal = noop;

    this.readyState = WebSocket.CONNECTING;
    if (options.loggingEnabled !== undefined) {
      if (typeof (options.loggingEnabled) !== 'boolean') {
        throw new Error('loggingEnabled must be a boolean');
      }
    } else {
      options.loggingEnabled = false;
    }
    this.nativeHandle = new NativeWebSocket.WebsocketClientWorker(serverUri, this, options);
  }

  set onopen(f = noop) {
    this.onOpenCallbackInternal = f;
  }

  set onmessage(f = noop) {
    this.onMessageCallbackInternal = f;
  }

  set onclose(f = noop) {
    this.onCloseCallbackInternal = f;
  }

  set onerror(f = noop) {
    this.onErrorCallbackInternal = f;
  }

  set onlog(f = noop) {
    this.onLogCallbackInternal = f;
  }

  close(code = 1000, reason = '') {
    if (typeof code !== 'number') {
      throw new Error('close must be called with a valid `close` code');
    }

    this.nativeHandle.close(code, reason);
    this.readyState = WebSocket.CLOSING;
  }

  onOpenCallback() {
    this.readyState = WebSocket.OPEN;
    this.onOpenCallbackInternal();
  }

  onMessageCallback(msg) {
    this.onMessageCallbackInternal({data: msg});
  }

  onCloseCallback(code, reason) {
    this.readyState = WebSocket.CLOSED;
    this.onCloseCallbackInternal({code: code, reason: reason});
    this.nativeHandle = null;
  }

  onErrorCallback(code, reason) {
    const event = {code: code, reason: reason};
    this.onErrorCallbackInternal(event);
    this.onCloseCallback(code, reason);
  }

  onLogCallback(level, logMsg) {
    this.onLogCallbackInternal(level, logMsg);
  }

  /**
   * @todo in order to fully comply with "ws" API, we would need to support
   * callbacks
   */
  send(message, options = {}) {
    if (typeof message === 'number') {
      message = message.toString();
    }

    var isBinary = true;

    if (options.binary !== undefined) {
      isBinary = options.binary;
    } else {
      isBinary = typeof message !== 'string';
    }

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
