#include "FileHandleMock.h"
#include "TestHarness.h"
#include "mock.h"
#include <string.h>

namespace mbed {
    
extern void trace(char *string, int data);

Callback<void()> FileHandleMock::_sigio_cb;    

void FileHandleMock::io_control(const io_control_t &control)
{    
    switch (control.io_type) {
        case IO_TYPE_SIGNAL_GENERATE:
            _sigio_cb();
            break;
        default:
            MBED_ASSERT(false);
            break;
    }
}


ssize_t FileHandleMock::read(void *buffer, size_t size)
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return -1;
    }
    
    if (mock->input_param[0].compare_type == MOCK_COMPARE_TYPE_VALUE) {
        CHECK_EQUAL(mock->input_param[0].param, size);
    }   
    
    if (mock->output_param[0].param != NULL) {
        memmove(buffer, mock->output_param[0].param, mock->output_param[0].len);
    }
    
    return (ssize_t)mock->return_value;   
}
    
    
ssize_t FileHandleMock::write(const void *buffer, size_t size)
{    
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return -1;
    }

    if (mock->input_param[1].compare_type == MOCK_COMPARE_TYPE_VALUE) {
        CHECK_EQUAL(mock->input_param[1].param, size);
    }       
    if (mock->input_param[0].compare_type == MOCK_COMPARE_TYPE_VALUE) {
        if (memcmp((void *)(mock->input_param[0].param), buffer, size) != 0) {        
            uint8_t expected;
            uint8_t actual;
            for (uint8_t i = 0; i < size; ++i) {
                actual   = ((uint8_t*)buffer)[i];
                expected = ((uint8_t*)(mock->input_param[0].param))[i];
                
                trace("expected: ", expected);                
                trace("actual: ", actual);
                trace("index: ", i);
            }
            trace("magic: ", mock->magic);
            FAIL("FAILURE:");        
        }
    }
    
    return (ssize_t)mock->return_value;   
}


short FileHandleMock::poll(short events) const
{
    FAIL("FAILURE: not supported");
    return 0;
}
    
    
void FileHandleMock::sigio(Callback<void()> func)
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
    }
    
    _sigio_cb = func;
}
    

off_t FileHandleMock::seek(off_t offset, int whence)
{
    FAIL("FAILURE: not supported");
    return 0;
}


int FileHandleMock::close()
{
    FAIL("FAILURE: not supported");
    return 0;
}
    
} // namespace mbed
