/*
 * Copyright (c) 2019, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
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

#include "SIMCom_SIM7020_CellularStack.h"
#include "CellularLog.h"

using namespace mbed;

SIMCom_SIM7020_CellularStack::SIMCom_SIM7020_CellularStack(ATHandler       &atHandler,
                                                           int              cid,
                                                           nsapi_ip_stack_t stack_type) :
                                                           AT_CellularStack(atHandler, cid, stack_type)
{
    _at.set_urc_handler("+CSONMI:", mbed::Callback<void()>(this, &SIMCom_SIM7020_CellularStack::urc_csonmi));
}

SIMCom_SIM7020_CellularStack::~SIMCom_SIM7020_CellularStack()
{

}

nsapi_error_t SIMCom_SIM7020_CellularStack::socket_listen(nsapi_socket_t handle, int backlog)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SIMCom_SIM7020_CellularStack::socket_accept(void *server, void **socket, SocketAddress *addr)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

nsapi_error_t SIMCom_SIM7020_CellularStack::socket_connect(nsapi_socket_t handle, const SocketAddress &address)
{
    return NSAPI_ERROR_NO_CONNECTION;
}

void SIMCom_SIM7020_CellularStack::urc_csonmi()
{
    MBED_ASSERT(false);
}

int SIMCom_SIM7020_CellularStack::get_max_socket_count()
{
    return 5;
}

bool SIMCom_SIM7020_CellularStack::is_protocol_supported(nsapi_protocol_t protocol)
{
    return (protocol == NSAPI_UDP || protocol == NSAPI_TCP);
}

nsapi_error_t SIMCom_SIM7020_CellularStack::socket_close_impl(int sock_id)
{
    MBED_ASSERT(false);
}

void SIMCom_SIM7020_CellularStack::handle_open_socket_response(int &modem_connect_id, int &err)
{
    MBED_ASSERT(false);
}

nsapi_error_t SIMCom_SIM7020_CellularStack::create_socket_impl(CellularSocket *socket)
{
    MBED_ASSERT(false);
}

nsapi_size_or_error_t SIMCom_SIM7020_CellularStack::socket_sendto_impl(CellularSocket      *socket,
                                                                       const SocketAddress &address,
                                                                       const void          *data,
                                                                       nsapi_size_t         size)
{
    MBED_ASSERT(false);
}

nsapi_size_or_error_t SIMCom_SIM7020_CellularStack::socket_recvfrom_impl(CellularSocket *socket,
                                                                         SocketAddress  *address,
                                                                         void           *buffer,
                                                                         nsapi_size_t    size)
{
    MBED_ASSERT(false);
}
