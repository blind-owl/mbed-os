#include "SemaphoreMock.h"
#include "TestHarness.h"
#include "mock.h"

namespace mbed {

int32_t SemaphoreMock::wait()
{
    mock_t *mock = mock_open(__FUNCTION__);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return -1;
    }
    
    if (mock->func != NULL) {
        mock->func(mock->func_context);        
    }
    
    return (int32_t)mock->return_value;
}

osStatus SemaphoreMock::release(void)
{
    mock_t *mock = mock_open(__FUNCTION__);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return (osStatus)~osOK;
    }
    
    return (osStatus)mock->return_value; 
}
 
 
} // namespace mbed
