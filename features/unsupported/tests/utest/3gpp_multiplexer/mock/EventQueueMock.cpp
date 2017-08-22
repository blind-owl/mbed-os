#include "EventQueueMock.h"
#include "TestHarness.h"
#include "mock.h"

namespace mbed {
    
extern void trace(char *string, int data);

Callback<void()> EventQueueMock::_call_cb;    
Callback<void()> EventQueueMock::_call_in_cb;

void EventQueueMock::io_control(const io_control_t &control)
{
    switch (control.io_type) {
        case IO_TYPE_DEFERRED_CALL_GENERATE:
            _call_cb();
            break;
        case IO_TYPE_TIMEOUT_GENERATE:            
//trace("IO_TYPE_TIMEOUT_GENERATE: ", 0);                           
            _call_in_cb();
            break;
        default:
            MBED_ASSERT(false);
            break;
    }    
}
 
 
int EventQueueMock::call(Callback<void()> func)
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return -1;
    }
    
    _call_cb = func;
    
    return mock->return_value;
}


int EventQueueMock::call_in(int ms, Callback<void()> func)
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
        return -1;
    }
    
    if (mock->input_param[0].compare_type == MOCK_COMPARE_TYPE_VALUE) {
        CHECK_EQUAL(ms, mock->input_param[0].param);
    }   
    
    _call_in_cb = func;
    
//trace("EventQueueMock::call_in", 0);

    return mock->return_value;
}


void EventQueueMock::cancel(int id)
{
    mock_t *mock = mock_open(__FUNCTION__, MockInvalidateTypeYes);
    if (mock == NULL) {
        FAIL("FAILURE: No mock object found!");
    }
    
    if (mock->input_param[0].compare_type == MOCK_COMPARE_TYPE_VALUE) {
        CHECK_EQUAL(id, mock->input_param[0].param);
    }       
}

} // namespace mbed
