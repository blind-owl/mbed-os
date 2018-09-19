#include "mbed_mux_data_service.h"
#include "mbed_mux.h"

namespace mbed {
   
ssize_t MuxDataService::write(const void* buffer, size_t size)
{
    return Mux::user_data_tx(_dlci, buffer, size);
}
    

ssize_t MuxDataService::read(void *buffer, size_t size)
{
    return Mux::user_data_rx(buffer, size);
}
    
    
off_t MuxDataService::seek(off_t offset, int whence)
{
    MBED_ASSERT(false);
    return 0;
}
    

int MuxDataService::close()
{
    MBED_ASSERT(false);    
    return 0;
}
    

void MuxDataService::sigio(Callback<void()> func)
{
    _sigio_cb = func;
}
    
} // namespace mbed
