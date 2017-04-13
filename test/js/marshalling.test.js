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

const testRootDir = require('path').resolve(__dirname, '..');
const marshallingTest =
    require('bindings')({module_root: testRootDir, bindings: 'marshallingTest'});

function runScalarTest(testCase) {
  const testName = 'scalar: ' + typeof testCase.value + ' "' + testCase.value + '"';
  test(testName, t => {
    const returnValue = marshallingTest[testCase.func]({value: testCase.value});
    t.deepEqual(returnValue, testCase.value);
  });
}

function runVectorTest(testCase) {
  const testName = 'vector: ' + typeof testCase.value + ' "' + testCase.value + '"';
  const expectedValue = Array(10).fill(testCase.value);
  test(testName, t => {
    const returnValue = marshallingTest[testCase.func + 'Vector']({value: expectedValue});
    t.deepEqual(returnValue, expectedValue);
  });
}

const testCases = [
  {value: 'hello world', func: 'getString'},
  {value: 12345, func: 'getInteger'},
  {value: 987.654, func: 'getDouble'},
  {value: true, func: 'getBool'},
];

testCases.forEach(testCase => runScalarTest(testCase));
testCases.forEach(testCase => runVectorTest(testCase));

test('callback', t => {
  t.plan(1);
  const callbackFunction = () => {
    t.pass();
  };
  marshallingTest.invokeCallback({callback: callbackFunction});
});
