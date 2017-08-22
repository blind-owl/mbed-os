#include "SemaphoreMock.h"
#include "TestHarness.h"
#include "mock.h"

namespace mbed {

int32_t SemaphoreMock::wait()
{
#if 0    
    UT_PRINT(__FUNCTION__);            
#endif    
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeNo);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return -1;
    }
       
    /* Execute, if exists, programmed callback and only invalidate mock after that.*/
    if (mock->func != NULL) {        
        mock->func(mock->func_context);
    }
   
#if 0        
    UT_PRINT("CALL:mock_invalidate");            
#endif        
    mock_invalidate(__FUNCTION__);
        
    return (int32_t)mock->return_value;
}

osStatus SemaphoreMock::release(void)
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return (osStatus)~osOK;
    }

    return (osStatus)mock->return_value; 
}
 
 
} // namespace mbed
