
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

#ifndef MBED_EVENTQUEUEMOCK_H
#define MBED_EVENTQUEUEMOCK_H

#include "Callback.h"
#include <stdint.h>

namespace mbed {

class EventQueueMock {
public:
    
    typedef enum 
    {
      IO_TYPE_DEFERRED_CALL_GENERATE = 0,
      IO_TYPE_TIMEOUT_GENERATE,
      IO_TYPE_MAX
    } IoType;
       
    typedef struct
    {
        IoType io_type;        
    } io_control_t;
    
    static void io_control(const io_control_t &control);    
    
    static int call(Callback<void()> func);
    static int call_in(int ms, Callback<void()> func);
    static void cancel(int id);    
    
private:

    static Callback<void()> _call_cb;
    static Callback<void()> _call_in_cb;        
};

} // namespace mbed

#endif
