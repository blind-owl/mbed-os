/* mbed Microcontroller Library
* Copyright (c) 2006-2017 ARM Limited
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

#ifndef MUXDATASERVICE_H
#define MUXDATASERVICE_H

#include <stdint.h>
#include "mbed_mux_data_service_base.h"

#define MUX_DLCI_INVALID_ID 0   /* Invalid DLCI ID. Used to invalidate MuxDataService object. */

namespace mbed {

class MuxDataService : public MuxDataServiceBase {

friend class Mux;
public:

    /** Enqueue user data for transmission.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service tx. Supplied buffer can be
     *         reused/freed upon call return.
     *
     *  @param buffer Begin of the user data.
     *  @param size   The number of bytes to write.
     *  @return       The number of bytes written.
     */
    virtual ssize_t write(const void* buffer, size_t size);

    /** Read user data into a buffer.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service rx.
     *
     *  @param buffer The buffer to read in to.
     *  @param size   The number of bytes to read.
     *  @return       The number of bytes read, -EAGAIN if no data availabe for read.
     */
    virtual ssize_t read(void *buffer, size_t size);

    /** Not supported by the implementation. */
    virtual off_t seek(off_t offset, int whence = SEEK_SET);

    /** Not supported by the implementation. */
    virtual int close();

    /** Register a callback on completion of enqueued write and read operations.
     *
     *  @note: The registered callback is called within thread context supplied in @ref eventqueue_attach.
     *
     *  @param func Function to call upon event generation.
     */
    virtual void sigio(Callback<void()> func);

    /** Constructor. */
    MuxDataService() : _dlci(MUX_DLCI_INVALID_ID) {};

private:

    /* Deny copy constructor. */
    MuxDataService(const MuxDataService &obj);

    /* Deny assignment operator. */
    MuxDataService &operator=(const MuxDataService &obj);

    uint8_t _dlci;     /* DLCI number. Valid range 1 - 63. */
};

} // namespace mbed

#endif
