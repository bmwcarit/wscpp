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

import test from 'ava';
const Setup = require('./setup.helper.js');


test.cb('close code and reason are sent to server', t => {
  Setup.initNonTlsCommunication().then((setup) => {

    const expectedCode = 4000;
    const expectedReason = 'test-reason';

    setup.server.on('connection', (ws) => {
      ws.on('close', (code, reason) => {
        t.is(code, expectedCode);
        t.is(reason, expectedReason);
        t.end();
      });
    });

    setup.client.onopen = () => {
      setup.client.close(expectedCode, expectedReason);
    };
  });
});
