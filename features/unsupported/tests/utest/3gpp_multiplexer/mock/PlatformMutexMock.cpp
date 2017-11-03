#include "PlatformMutexMock.h"
#include "TestHarness.h"
#include "mock.h"

namespace mbed {

void PlatformMutexMock::lock()
{
#if 0    
    UT_PRINT(__FUNCTION__);            
#endif    
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
    }
}

void PlatformMutexMock::unlock()
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
    } 
}
 
 
} // namespace mbed
