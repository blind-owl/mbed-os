/*
 * Copyright (c) 2018, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <string.h>
#include "mbed_mux.h"
#include "FileHandle.h"

#if 0
using namespace mbed;
using namespace events;
#endif

// AStyle ignored as the definition is not clear due to preprocessor usage
// *INDENT-OFF*
class TestMux : public testing::Test {
protected:

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};
// *INDENT-ON*
#if 0
class MockFileHandle : public mbed::FileHandle {

public:
    MOCK_METHOD0(size, off_t());
}; // virtual off_t size();
#endif 
/*
 * TC - Ensure proper behaviour when channel is opened and multiplexer control channel is not open
 *
 * Test sequence:
 * - Send open multiplexer control channel request message
 * - Receive open multiplexer control channel response message
 * - Send open user channel request message
 * - Receive open user channel response message
 * - Generate channel open callbck with a  valid FileHandle
 *
 * Expected outcome:
 * - As specified above
 */
TEST_F(TestMux, channel_open_mux_not_open)
{
    mbed::Mux3GPP obj;
//    mbed::Mux3GPP::eventqueue_attach(&eq_mock);
#if 0
    mbed::FileHandleMock fh_mock;
    mbed::EventQueueMock eq_mock;

    mbed::Mux3GPP::eventqueue_attach(&eq_mock);

    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");
    CHECK(mock_sigio != NULL);
    mbed::Mux3GPP::serial_attach(&fh_mock);

    MuxCallbackTest callback;
    mbed::Mux3GPP::callback_attach(mbed::Callback<void(MuxBase::event_context_t &)>(&callback, &MuxCallbackTest::channel_open_run),
                               mbed::MuxBase::CHANNEL_TYPE_AT);

    mux_self_iniated_open(callback, FRAME_TYPE_UA);

    /* Validate Filehandle generation. */
    CHECK(callback.is_callback_called());
    FileHandle *fh = callback.file_handle_get();
    CHECK(fh != NULL);
#endif
}
