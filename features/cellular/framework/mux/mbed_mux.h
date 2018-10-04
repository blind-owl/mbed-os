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

#ifndef MUX_H
#define MUX_H

#include <stdint.h>
#include "platform/platform.h"
#include "Callback.h"
#include "mbed_mux_base.h"
#include "mbed_mux_data_service.h"
#include "nsapi_types.h"
#include "PlatformMutex.h"
#include "EventQueue.h"

#define MUX_CRC_TABLE_LEN         256u /* CRC table length in number of bytes. */

#ifndef MBED_CONF_MUX_DLCI_COUNT
#define MBED_CONF_MUX_DLCI_COUNT  3u   /* Number of supported DLCI IDs. */
#endif
#ifndef MBED_CONF_MUX_BUFFER_SIZE
#define MBED_CONF_MUX_BUFFER_SIZE 31u  /* Size of TX/Rx buffers in number of bytes. */
#endif

/* More RAM needs to allocated if more than 4 DLCI ID to be supported see @ref tx_callback_context for details. */
MBED_STATIC_ASSERT(MBED_CONF_MUX_DLCI_COUNT <= 4u, "");


/* @todo:
I assume that we need to export some kind of #defines for EVENT_SIZE and MAX_EVENT_COUNT (max number of events that can
be queued at the same time by the module inside EventQueue, so that the application designer can calculate the RAM
storage requirements correctly at compile time.
*/

namespace mbed {

class FileHandle;
class Mux3GPP : protected MuxBase {
friend class MuxDataService3GPP;
public:

    /** Module init. */
    static void module_init();

    /** Open multiplexer channel.
     *
     *
     * @return NSAPI_ERROR_OK          Operation accepted for execution. Completion callback will be called.
     * @return NSAPI_ERROR_IN_PROGRESS Operation not accpeted for execution. Operation allready inprogress, completion
     *                                 callback will not be called.
     * @return NSAPI_ERROR_NO_MEMORY   Operation not accepted for execution. All available channels are allready opened,
     *                                 completion callback will not be called.
     */
    static nsapi_error channel_open();

    /** Attach serial interface to the object.
     *
     *  @param serial Serial interface to be used.
     */
    static void serial_attach(FileHandle *serial);

    /** Attach eventqueue interface to the object.
     *
     *  @param event_queue Event queue interface to be used.
     */
    static void eventqueue_attach(events::EventQueue *event_queue);

    /** Attach @ref channel_open operation completion callback function.
     *
     *  @param func @ref channel_open operation completion callback function.
     *  @param type Not used by the implementation.
     */
    static void callback_attach(Callback<void(event_context_t &)> func, ChannelType type) {_cb_func = func;}

private:

    /* Definition for Rx event type. */
    typedef enum
    {
        RX_READ = 0,
        RX_RESUME,
        RX_EVENT_MAX
    } RxEvent;

    /* Definition for Tx state machine. */
    typedef enum
    {
        TX_IDLE = 0,
        TX_RETRANSMIT_ENQUEUE,
        TX_RETRANSMIT_DONE,
        TX_INTERNAL_RESP,
        TX_NORETRANSMIT,
        TX_STATE_MAX
    } TxState;

    /* Definition for Rx state machine. */
    typedef enum
    {
        RX_FRAME_START = 0,
        RX_HEADER_READ,
        RX_TRAILER_READ,
        RX_SUSPEND,
        RX_STATE_MAX
    } RxState;

    /* Definition for frame type within rx path. */
    typedef enum
    {
        FRAME_RX_TYPE_SABM = 0,
        FRAME_RX_TYPE_UA,
        FRAME_RX_TYPE_DM,
        FRAME_RX_TYPE_DISC,
        FRAME_RX_TYPE_UIH,
        FRAME_RX_TYPE_NOT_SUPPORTED,
        FRAME_RX_TYPE_MAX
    } FrameRxType;

    /* Definition for frame type within tx path. */
    typedef enum
    {
        FRAME_TX_TYPE_SABM = 0,
        FRAME_TX_TYPE_DM,
        FRAME_TX_TYPE_UIH,
        FRAME_TX_TYPE_MAX
    } FrameTxType;

    /** Registered time-out expiration event. */
    static void on_timeout();

    /** Registered deferred call event in safe (thread context) supplied in @ref eventqueue_attach. */
    static void on_deferred_call();

    /** Registered sigio callback from FileHandle. */
    static void on_sigio();

    /** Calculate fcs.
     *
     *  @param buffer    Input buffer.
     *  @param input_len Input length in number of bytes.
     *
     *  @return Calculated fcs.
     */
    static uint8_t fcs_calculate(const uint8_t *buffer,  uint8_t input_len);

    /** Construct sabm request message.
     *
     *  @param dlci_id ID of the DLCI to establish
     */
    static void sabm_request_construct(uint8_t dlci);

    /** Construct dm response message.
     */
    static void dm_response_construct();

    /** Construct user information frame.
     *
     *  @param dlci_id ID of the DLCI to establish
     */
    static void user_information_construct(uint8_t dlci_id, const void *buffer, size_t size);

    /** Do write operation if pending data available.
     */
    static void write_do();

    /** Generate Rx event.
     *
     *  @param event Rx event
     */
    static void rx_event_do(RxEvent event);

    /** Rx event frame start read state.
     *
     *  @return Number of bytes read, -EAGAIN if no data available.
     */
    static ssize_t on_rx_read_state_frame_start();

    /** Rx event header read state.
     *
     *  @return Number of bytes read, -EAGAIN if no data available.
     */
    static ssize_t on_rx_read_state_header_read();

    /** Rx event trailer read state.
     *
     *  @return Number of bytes read, -EAGAIN if no data available.
     */
    static ssize_t on_rx_read_state_trailer_read();

    /** Rx event suspend read state.
     *
     *  @return Number of bytes read, -EAGAIN if no data available.
     */
    static ssize_t on_rx_read_state_suspend();

    /** Process received SABM frame. */
    static void on_rx_frame_sabm();

    /** Process received UA frame. */
    static void on_rx_frame_ua();

    /** Process received DM frame. */
    static void on_rx_frame_dm();

    /** Process received DISC frame. */
    static void on_rx_frame_disc();

    /** Process received UIH frame. */
    static void on_rx_frame_uih();

    /** Process received frame, which is not supported. */
    static void on_rx_frame_not_supported();

    /** Process valid received frame. */
    static void valid_rx_frame_decode();

    /** SABM frame tx path post processing. */
    static void on_post_tx_frame_sabm();

    /** DM frame tx path post processing. */
    static void on_post_tx_frame_dm();

    /** UIH frame tx path post processing. */
    static void on_post_tx_frame_uih();

    /** Resolve rx frame type.
     *
     *  @return Frame type.
     */
    static Mux3GPP::FrameRxType frame_rx_type_resolve();

    /** Resolve tx frame type.
     *
     *  @return Frame type.
     */
    static Mux3GPP::FrameTxType frame_tx_type_resolve();

    /** Begin the frame retransmit sequence. */
    static void frame_retransmit_begin();

    /** TX state entry functions. */
    static void tx_retransmit_enqueu_entry_run();
    static void tx_retransmit_done_entry_run();
    static void tx_idle_entry_run();
    static void tx_internal_resp_entry_run();
    static void tx_noretransmit_entry_run();
    typedef void (*tx_state_entry_func_t)();

    /** TX state exit function. */
    static void tx_idle_exit_run();
    typedef void (*tx_state_exit_func_t)();

    /** Change Tx state machine state.
     *
     *  @param new_state  State to transit.
     *  @param entry_func State entry function.
     *  @param exit_func  State exit function.
     */
    static void tx_state_change(TxState new_state, tx_state_entry_func_t entry_func, tx_state_exit_func_t exit_func);

    /** RX state entry functions. */
    static void rx_header_read_entry_run();
    typedef void (*rx_state_entry_func_t)();

    /** Null action function. */
    static void null_action();

    /** Change Rx state machine state.
     *
     *  @param new_state  State to transit.
     *  @param entry_func State entry function.
     */
    static void rx_state_change(RxState new_state, rx_state_entry_func_t entry_func);

    /** Begin DM frame transmit sequence. */
    static void dm_response_send();

    /** Append DLCI ID to storage.
     *
     *  @param dlci_id ID of the DLCI to append.
     */
    static void dlci_id_append(uint8_t dlci_id);

    /** Get file handle based on DLCI ID.
     *
     *  @param dlci_id ID of the DLCI used as the key
     *
     *  @return Valid object reference or NULL if not found.
     */
    static MuxDataService3GPP *file_handle_get(uint8_t dlci_id);

    /** Evaluate is DLCI ID in use.
     *
     *  @param dlci_id ID of the DLCI yo evaluate
     *
     *  @return True if in use, false otherwise.
     */
    static bool is_dlci_in_use(uint8_t dlci_id);

    /** Evaluate is DLCI ID queue full.
     *
     *  @return True if full, false otherwise.
     */
    static bool is_dlci_q_full();

    /** Begin pending self iniated multiplexer open sequence. */
    static void pending_self_iniated_mux_open_start();

    /** Begin pending self iniated DLCI establishment sequence. */
    static void pending_self_iniated_dlci_open_start();

    /** Begin pending peer iniated DLCI establishment sequence.
     *
     *  @param dlci_id ID of the DLCI to establish.
     */
    static void pending_peer_iniated_dlci_open_start(uint8_t dlci_id);

    /** Enqueue user data for transmission.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service tx. Supplied buffer can be
     *         reused/freed upon call return.
     *
     *  @param dlci_id ID of the DLCI to use.
     *  @param buffer  Begin of the user data.
     *  @param size    The number of bytes to write.
     *  @return        The number of bytes written, negative error on failure.
     */
    static ssize_t user_data_tx(uint8_t dlci_id, const void* buffer, size_t size);

    /** Read user data into a buffer.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service rx.
     *
     *  @param buffer The buffer to read in to.
     *  @param size   The number of bytes to read.
     *  @return       The number of bytes read, -EAGAIN if no data availabe for read.
     */
    static ssize_t user_data_rx(void* buffer, size_t size);

    /** Check for poll event flags
     *
     * @return Bitmask of poll events (POLLIN/POLLOUT) that have occurred.
     */
    static short poll();

    /** Clear TX callback pending bit.
     *
     *  @param bit Bit to clear.
     */
    static void tx_callback_pending_bit_clear(uint8_t bit);

    /** Set TX callback pending bit for supplied DLCI ID.
     *
     *  @param dlci_id DLCI ID for bit to set.
     */
    static void tx_callback_pending_bit_set(uint8_t dlci_id);

    /** Advance the current TX callback index bit.
     *
     *  @return The current TX callback index bit after advanced.
     */
    static uint8_t tx_callback_index_advance();

    /** Get the TX callback pending bitmask.
     *
     *  @return TX callback pending bitmask.
     */
    static uint8_t tx_callback_pending_mask_get();

    /** Dispatch TX callback based on supplied bit.
     *
     *  @param bit Bit indetifier of callback to dispatch.
     */
    static void tx_callback_dispatch(uint8_t bit);

    /** Run main processing loop for resolving pending TX callbacks and dispatching them if they exists.
     */
    static void tx_callbacks_run();

    /** Get data service object based on supplied bit id.
     *
     *  @param bit Bit indetifier of data service object to get.
     *  @return    Data service object reference.
     */
    static MuxDataService3GPP& tx_callback_lookup(uint8_t bit);

    /** Get minimum of 2 supplied paramaters.
     *
     *  @param size_1 1st param for comparisation.
     *  @param size_2 2nd param for comparisation.
     *  @return       Minimum of supplied paramaters.
     */
    static size_t min(uint8_t size_1, size_t size_2);

    /** Enqueue callback to event queue.
     */
    static void event_queue_enqueue();

    /** Verify is FCS valid in RX frame.
     *
     *  @return True upon valid FCS, false otherwise.
     */
    static bool is_rx_fcs_valid();

    /** Dispatch completion function for running user operation.
     *
     *  @param fh FileHandle supplied to the completion function.
     */
    static void operation_complete_dispatch(FileHandle *fh);

    /* Deny object creation. */
    Mux3GPP();

    /* Deny copy constructor. */
    Mux3GPP(const Mux3GPP& obj);

    /* Deny assignment operator. */
    Mux3GPP& operator=(const Mux3GPP& obj);

    /* Definition for Tx context type. */
    typedef struct
    {
        int timer_id;                   /* Timer id. */
        union {
            uint32_t align_4_byte;                      /* Force 4-byte alignment. */
            uint8_t  buffer[MBED_CONF_MUX_BUFFER_SIZE]; /* Rx buffer. */
        };
        uint8_t retransmit_counter;     /* Frame retransmission counter. */
        uint8_t bytes_remaining;        /* Bytes remaining in the buffer to write. */
        uint8_t offset;                 /* Offset in the buffer where to write from. */
        uint8_t tx_callback_context;    /* Context for the TX callback dispatching logic as follows:
                                           - 4 LO bits contain the pending callback mask
                                           - 4 HI bits contain the current bit used for masking */
        TxState tx_state;               /* Tx state machine current state. */

    } tx_context_t;

    /* Definition for Rx context type. */
    typedef struct
    {
        union {
            uint32_t align_4_byte;                      /* Force 4-byte alignment. */
            uint8_t  buffer[MBED_CONF_MUX_BUFFER_SIZE]; /* Rx buffer. */
        };
        uint8_t offset;         /* Offset in the buffer where to read to. */
        uint8_t read_length;    /* Amount to read in number of bytes. */
        RxState rx_state;       /* Rx state machine current state. */
    } rx_context_t;

    /* Definition for state type. */
    typedef struct
    {
        uint8_t is_mux_open              : 1; /* True when multiplexer is open. */

        // @todo: depricate below as we could use is_mux_open state?
        uint8_t is_mux_open_pending      : 1; /* True when multiplexer open is pending. */

        uint8_t is_dlci_open_pending     : 1; /* True when DLCI open is pending. */
        uint8_t is_tx_callback_context   : 1; /* True when current context is TX callback context. */
        uint8_t is_user_tx_pending       : 1; /* True when user TX is pending. */
        uint8_t is_op_complete_context   : 1; /* True when current context is operation complete callback context. */
    } state_t;

    static uint8_t             _is_deferred_call_enqueued;             /* State variable to determine is deferred call enqueued or not.*/
    static FileHandle         *_serial;                                /* Serial used. */
    static events::EventQueue *_event_q;                               /* Event queue used. */
    static PlatformMutex       _mutex;                                 /* Mutex used. */
    static MuxDataService3GPP  _mux_objects[MBED_CONF_MUX_DLCI_COUNT]; /* Number of supported DLCIs. */
    static tx_context_t        _tx_context;                            /* Tx context. */
    static rx_context_t        _rx_context;                            /* Rx context. */
    static state_t             _state;                                 /* General state context. */
    static const uint8_t       _crctable[MUX_CRC_TABLE_LEN];           /* CRC table used for frame FCS. */

    static uint8_t                     _dlci;                       /* User channel id. */
    static Callback<void(event_context_t &)> _cb_func;                    /* @ref channel_open completion function. */
};

} // namespace mbed

#endif
