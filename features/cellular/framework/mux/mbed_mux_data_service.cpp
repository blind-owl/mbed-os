#include "mbed_mux_data_service.h"
#include "mbed_mux.h"

namespace mbed {

Mux3GPP *MuxDataService3GPP::_mux = NULL;

ssize_t MuxDataService3GPP::write(const void* buffer, size_t size)
{
    return _mux->user_data_tx(_dlci, buffer, size);
}


ssize_t MuxDataService3GPP::read(void *buffer, size_t size)
{
    return _mux->user_data_rx(buffer, size);
}


short MuxDataService3GPP::poll(short events) const
{
    return _mux->poll();
}


off_t MuxDataService3GPP::seek(off_t offset, int whence)
{
    MBED_ASSERT(false);
    return 0;
}


int MuxDataService3GPP::close()
{
    MBED_ASSERT(false);
    return 0;
}


void MuxDataService3GPP::sigio(Callback<void()> func)
{
    _sigio_cb = func;
}

} // namespace mbed
