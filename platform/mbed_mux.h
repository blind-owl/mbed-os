
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
#include "FileHandle.h"
#include "SemaphoreMock.h"

#if 0
#include "rtos/Semaphore.h"
#endif // 0
/*
#define MBED_CONF_RTOS_PRESENT
#include "mbed.h"
*/

#define MUX_DLCI_INVALID_ID 0u      /* Invalid DLCI ID. Used to invalidate MuxDataService object. */
#define MUX_CRC_TABLE_LEN   256u    /* CRC table length in number of bytes. */

#ifndef MBED_CONF_MUX_DLCI_COUNT
#define MBED_CONF_MUX_DLCI_COUNT    3u
#endif
#ifndef MBED_CONF_BUFFER_SIZE
#define MBED_CONF_BUFFER_SIZE       31u
#endif 

/* @todo:
I assume that we need to export some kind of #defines for EVENT_SIZE and MAX_EVENT_COUNT (max number of events that can 
be queued at the same time by the module inside EventQueue, so that the application designer can calculate the RAM 
storage requirements correctly at compile time.
*/

namespace mbed {

class MuxDataService : public FileHandle {
friend class Mux;    
public:

    /** Enqueue user data for transmission. 
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service tx. Supplied buffer can be 
     *         reused/freed upon call return.
     * 
     *  @param buffer   Begin of the user data.
     *  @param size     The number of bytes to write.
     *  @return         The number of bytes written.
     */
    virtual ssize_t write(const void* buffer, size_t size);
       
    /** Read user data into a buffer.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service rx. 
     *
     *  @param buffer   The buffer to read in to.
     *  @param size     The number of bytes to read.
     *  @return         The number of bytes read, -EAGAIN if no data availabe for read.
     */
    virtual ssize_t read(void *buffer, size_t size); 
    
    /** Not supported by the implementation. 
     *
     * @todo: need decide what is the proper mbed way out of the box functionality(ASSERT, error code return etc.) 
     */    
    virtual off_t seek(off_t offset, int whence = SEEK_SET);
    
    /** Not supported by the implementation. 
     *
     * @todo: need decide what is the proper mbed way out of the box functionality(ASSERT, error code return etc.) 
     */        
    virtual int close();
    
    /** Register a callback on completion of enqueued write and read operations. 
     *
     *  @note: The registered callback is called within thread context supplied in @ref eventqueue_attach. 
     *
     *  @param func Function to call upon event generation.
     */
    virtual void sigio(Callback<void()> func);
    
    /** Constructor. */
    MuxDataService() : dlci(MUX_DLCI_INVALID_ID) {};
    
private:

    /* Deny copy constructor. */
    MuxDataService(const MuxDataService& obj);
    
    /* Deny assignment operator. */    
    MuxDataService& operator=(const MuxDataService& obj);
        
    uint8_t dlci;               /* DLCI number. Valid range 1 - 63. */    
    Callback<void()> _sigio_cb; /* Registered signal callback. */        
};

class EventQueueMock;
class FileHandle;
class Mux {    
friend class MuxDataService;    
public:
    
/* Definition for multiplexer establishment status type. */
typedef enum
{
    MUX_ESTABLISH_SUCCESS = 0, /* Peer accepted the request. */
    MUX_ESTABLISH_REJECT,      /* Peer rejected the request. */
    MUX_ESTABLISH_TIMEOUT,     /* Timeout occurred for the request. */
    MUX_ESTABLISH_MAX          /* Enumeration upper bound. */
} MuxEstablishStatus;

    // @todo: update me
    static void module_init();
    
    /** Establish the multiplexer control channel.
     *
     *  @note: Relevant request specific parameters are fixed at compile time within multiplexer component.     
     *  @note: Call returns when response from the peer is received, timeout or write error occurs.
     *
     *  @param status Operation completion code.
     *
     *  @return 2   Operation completed, check @ref status for completion code.
     *  @return 1   Operation not started, control channel open allready in progress.     
     *  @return 0   Operation not started, multiplexer control channel allready open.    
     */
    static uint32_t mux_start(MuxEstablishStatus &status);
        
    /** Establish a DLCI.
     *
     *  @note: Relevant request specific parameters are fixed at compile time within multiplexer component.
     *  @note: Call returns when response from the peer is received or timeout occurs.
     * 
     *  @warning: Not allowed to be called from callback context.
     *
     *  @param dlci_id  ID of the DLCI to establish. Valid range 1 - 63. 
     *  @param status   Operation completion code.     
     *  @param obj      Valid object upon @ref status having @ref MUX_ESTABLISH_SUCCESS, NULL upon failure.     
     *
     *  @return 4   Operation completed, check @ref status for completion code.
     *  @return 3   Operation not started, DLCI establishment allready in progress.     
     *  @return 2   Operation not started, DLCI ID not in valid range.
     *  @return 1   Operation not started, no established multiplexer control channel exists.
     *  @return 0   Operation not started, @ref dlci_id, or all available DLCI ID resources, allready in use.
     */        
    static uint32_t dlci_establish(uint8_t dlci_id, MuxEstablishStatus &status, FileHandle **obj);
        
    /** Attach serial interface to the object.
     *
     *  @param serial Serial interface to be used.
     */        
    static void serial_attach(FileHandle *serial);

    /** Attach eventqueue interface to the object.
     *
     *  @param event_queue Event queue interface to be used.
     */            
    static void eventqueue_attach(EventQueueMock *event_queue);
    
    // @todo make these private if possible as not meant to be called by user
    /** Registered time-out expiration event. */
    static void on_timeout();    
    /** Registered deferred call event in safe (thread context) supplied in @ref eventqueue_attach. */
    static void on_deferred_call();    
    /** Registered sigio callback from FileHandle. */    
    static void on_sigio();

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

    /** Calculate fcs.
     * 
     *  @param buffer       Input buffer.
     *  @param input_len    Input length in number of bytes.
     * 
     *  @return Calculated fcs.
     */    
    static uint8_t fcs_calculate(const uint8_t *buffer,  uint8_t input_len);

    /** Construct sabm request message.
     * 
     *  @param dlci_id  ID of the DLCI to establish
     */    
    static void sabm_request_construct(uint8_t dlci);
    
    /** Construct dm response message.
     */            
    static void dm_response_construct();
       
    /** Construct user information frame.
     * 
     *  @param dlci_id  ID of the DLCI to establish
     */                
    static void user_information_construct(uint8_t dlci_id, const void *buffer, size_t size);
    
    /** Do write operation if pending data available.
     */
    static void write_do();
    static void rx_event_do(RxEvent event);
    
    static ssize_t on_rx_read_state_frame_start();
    static ssize_t on_rx_read_state_header_read();
    static ssize_t on_rx_read_state_trailer_read();
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
    static Mux::FrameRxType frame_rx_type_resolve();
    
    /** Resolve tx frame type. 
     * 
     *  @return Frame type.
     */                        
    static Mux::FrameTxType frame_tx_type_resolve();

    /** Begin the frame retransmit sequence. */
    static void frame_retransmit_begin();
    
    /** TX state entry functions. */
    static void tx_retransmit_enqueu_entry_run();  
    static void tx_retransmit_done_entry_run();  
    static void tx_idle_entry_run();   
    static void tx_internal_resp_entry_run();
    static void tx_noretransmit_entry_run();
    typedef void (*tx_state_entry_func_t)();       
    
    /** TX state exit functions. */    
    static void tx_idle_exit_run();
    typedef void (*tx_state_exit_func_t)();       
    
    /** Change Tx state machine state. 
     * 
     *  @param new_state    State to transit.
     *  @param entry_func   State entry function.
     */                
    static void tx_state_change(TxState new_state, tx_state_entry_func_t entry_func, tx_state_exit_func_t exit_func);
    
    static void rx_header_read_entry_run();
    typedef void (*rx_state_entry_func_t)();       
    
    static void null_action();
    
    /** Change Rx state machine state. 
     * 
     *  @param new_state    State to transit.
     */                
    static void rx_state_change(RxState new_state, rx_state_entry_func_t entry_func);   
 
    
    /** Begin DM frame transmit sequence. */    
    static void dm_response_send();
      
    /** Append DLCI ID to storage. 
     * 
     *  @param dlci_id  ID of the DLCI to append.
     */                    
    static void dlci_id_append(uint8_t dlci_id);
    
    /** Get file handle based on DLCI ID. 
     * 
     *  @param dlci_id  ID of the DLCI used as the key
     * 
     *  @return Valid object reference or NULL if not found.
     */                        
    static MuxDataService* file_handle_get(uint8_t dlci_id);
    
    /** Evaluate is DLCI ID in use. 
     * 
     *  @param dlci_id  ID of the DLCI yo evaluate
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
     *  @param dlci_id  ID of the DLCI to establish.
     */                
    static void pending_peer_iniated_dlci_open_start(uint8_t dlci_id);

    /** Enqueue user data for transmission. 
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service tx. Supplied buffer can be 
     *         reused/freed upon call return.
     * 
     *  @param dlci_id  ID of the DLCI to use.
     *  @param buffer   Begin of the user data.
     *  @param size     The number of bytes to write.
     *  @return         The number of bytes written, negative error on failure.
     */    
    static ssize_t user_data_tx(uint8_t dlci_id, const void* buffer, size_t size);
    
    /** Read user data into a buffer.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service rx. 
     *
     *  @param buffer   The buffer to read in to.
     *  @param size     The number of bytes to read.
     *  @return         The number of bytes read, -EAGAIN if no data availabe for read.
     */
    static ssize_t user_data_rx(void* buffer, size_t size);
       
    static void tx_callback_pending_bit_clear(uint8_t bit);
    static void tx_callback_pending_bit_set(uint8_t dlci_id);

    static uint8_t tx_callback_index_advance();
    static uint8_t tx_callback_pending_mask_get();
    static void tx_callback_dispatch(uint8_t bit);
    static MuxDataService& tx_callback_lookup(uint8_t bit);
    
    static size_t min(uint8_t size_1, size_t size_2);
    
    /** Get DLCI ID from the Rx buffer.
     *
     *  @return DLCI ID from the Rx buffer.
     */    
    static uint8_t rx_dlci_id_get();
    
    /* Deny object creation. */    
    Mux();
    
    /* Deny copy constructor. */
    Mux(const Mux& obj);
    
    /* Deny assignment operator. */    
    Mux& operator=(const Mux& obj);
      
    /* Definition for Tx context type. */
    typedef struct 
    {
        int     timer_id;                       /* Timer id. */
        TxState tx_state;                       /* Tx state machine current state. */
        uint8_t retransmit_counter;             /* Frame retransmission counter. */
        uint8_t bytes_remaining;                /* Bytes remaining in the buffer to write. */
        uint8_t offset;                         /* Offset in the buffer where to write from. */
        uint8_t buffer[MBED_CONF_BUFFER_SIZE];  /* Tx buffer. */
        
        uint8_t tx_callback_context;  // @todo: set me in init()          
       
    } tx_context_t;
          
    /* Definition for Rx context type. */
    typedef struct 
    {        
        RxState rx_state;                       /* Rx state machine current state. */
        uint8_t offset;                         /* Offset in the buffer where to read to. */
        uint8_t read_length;                    /* Amount to read in number of bytes. */        
        uint8_t buffer[MBED_CONF_BUFFER_SIZE];  /* Rx buffer. */
       
    } rx_context_t;    
    
    /* Definition for state type. */
    typedef struct
    {
        uint16_t is_mux_open :                       1;         /* True when multiplexer is open. */
        uint16_t is_mux_open_self_iniated_pending  : 1;
        uint16_t is_mux_open_self_iniated_running  : 1;
        uint16_t is_dlci_open_self_iniated_pending : 1;
        uint16_t is_dlci_open_self_iniated_running : 1;
        uint16_t is_user_thread_context            : 1;
        
        uint16_t is_tx_callback_context            : 1;
        uint16_t is_user_tx_pending                : 1; 
        
        uint16_t is_user_rx_ready                  : 1;
    } state_t;
    
    static FileHandle      *_serial;                                /* Serial used. */  
    static EventQueueMock  *_event_q;                               /* Event queue used. */  
//    static rtos::Semaphore  _semaphore;                             /* Semaphore. */
    static SemaphoreMock    _semaphore;
    static MuxDataService   _mux_objects[MBED_CONF_MUX_DLCI_COUNT]; /* Number of supported DLCIs (multiplexer 
                                                                       object pool) is fixed at compile time. */
    static tx_context_t     _tx_context;                            /* Tx context. */
    static rx_context_t     _rx_context;                            /* Rx context. */    
    static state_t          _state;                                 /* General state context. */
    static const uint8_t    _crctable[MUX_CRC_TABLE_LEN];           /* CRC table used for frame FCS. */
    
    static volatile uint8_t _establish_status;
    static volatile uint8_t _dlci_id;   
};

} // namespace mbed

#endif
