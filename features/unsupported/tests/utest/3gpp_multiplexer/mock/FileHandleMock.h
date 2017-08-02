
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

#ifndef MBED_FILEHANDLEMOCK_H
#define MBED_FILEHANDLEMOCK_H

#include "FileHandle.h"

namespace mbed {

class FileHandleMock : public FileHandle {
public:
    
    typedef enum 
    {
      IO_TYPE_SIGNAL_GENERATE = 0,
      IO_TYPE_MAX
    } IoType;
    
    typedef struct
    {
        IoType io_type;
    } io_control_t;
    
    static void io_control(const io_control_t &control);
    
    virtual ssize_t read(void *buffer, size_t size);
    virtual ssize_t write(const void *buffer, size_t size);
    virtual short poll(short events) const;   
    virtual void sigio(Callback<void()> func);
    
    virtual off_t seek(off_t offset, int whence = SEEK_SET);
    virtual int close();
private:

    static Callback<void()> _sigio_cb;   
};

} // namespace mbed

#endif
