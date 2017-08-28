
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
#define MBED_CONF_BUFFER_SIZE       64u
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
     *  @return         The number of bytes written, negative error on failure.
     */
    virtual ssize_t write(const void* buffer, size_t size);
    
    /** Equivalent to POSIX poll(). Returns bitmap of events available. 
     *
     *  @note: Events are self clearing, this meaning if this API is called again without new event being generated 
     *         event bit will be clear.
     *  
     *  @param events   Ignored by the implementation.
     *  @return POLLIN  Data should be availabe for read by @ref read.
     *  @return POLLOUT New enqueue operation should be possible @ref write.
     */
    virtual short poll(short events) const;    
    
    /** Read user data into a buffer.
     *
     *  @note: This is API is only meant to be used for the multiplexer (user) data service rx. 
     *
     *  @param buffer   The buffer to read in to.
     *  @param size     The number of bytes to read.
     *  @return         EAGAIN if no data availabe for read.
     *  @return         The number of bytes read, negative error on failure
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
    
    /** Register a callback on completion of enqueued write and read operations. The user is expected to use @ref poll
     *  to resolve correct event(s).
     *
     *  @note: The specified callback will be called upon generation of events defined in @ref poll
     *
     *  @note: The registered callback is called within thread context. 
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

class MuxCallback;
class EventQueueMock;
class FileHandle;
class Mux {    
public:

/* Definition for multiplexer event id type. */
typedef enum
{
    MUX_EVT_RX_OVERFLOW = 0,    /* Rx overflow. Rx buffer enqueue index is reset, which means that all the existing 
                                   data in the buffer will be overwritten. */
    MUX_EVT_ID_MAX              /* Enumeration upper bound. */
} MuxEventId;

/* Definition for multiplexer event type. */
#if 0
typedef struct
{
  MuxEventId id; /* Event id. */
} mux_event_t;
#endif // 0
    
/* Definition for multiplexer establishment status type. */
typedef enum
{
    MUX_ESTABLISH_SUCCESS = 0, /* Peer accepted the request. */
    MUX_ESTABLISH_REJECT,      /* Peer rejected the request. */
    MUX_ESTABLISH_TIMEOUT,     /* Timeout occurred for the request. */
    MUX_ESTABLISH_WRITE_ERROR, /* Write error occurred for the request. */    
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
     *  @return 1   Operation not started, peer or self initiated control channel open allready in progress.     
     *  @return 0   Operation not started, multiplexer control channel allready open.    
     */
    static uint32_t mux_start(MuxEstablishStatus &status);
        
    /** Establish a DLCI.
     *
     *  @note: Relevant request specific parameters are fixed at compile time within multiplexer component.
     *  @note: Call returns when response from the peer is received, timeout or write error occurs.
     *
     *  @param dlci_id  ID of the DLCI to establish. Valid range 1 - 63. 
     *  @param status   Operation completion code.     
     *  @param obj      Valid object upon @ref status having success, NULL upon failure.     
     *
     *  @return 3   Operation completed, check @ref status for completion code.
     *  @return 2   Operation not started, DLCI ID not in valid range.
     *  @return 1   Operation not started, no established multiplexer control channel exists.
     *  @return 0   Operation not started, @ref dlci_id, or all available DLCI ID resources, allready in use.
     *  
     * @todo: ADD Operation not started, peer or self initiated channel open for DLCI ID allready in progress.     
     */        
    static uint32_t dlci_establish(uint8_t dlci_id, MuxEstablishStatus &status, FileHandle **obj);
        
    /** Attach serial interface to the object.
     *
     *  @param serial Serial interface to be used.
     */        
    static void serial_attach(FileHandle *serial);
    
    /** Attach callback interface to the object.
     *
     *  @param callback Callback interface to be used.
     */        
    static void callback_attach(MuxCallback *callback);    
    
    /** Attach eventqueue interface to the object.
     *
     *  @param event_queue Event queue interface to be used.
     */            
    static void eventqueue_attach(EventQueueMock *event_queue);
    
    /** Registered time-out expiration event. */
    static void on_timeout();
    
    /** Registered deferred call event in safe (thread context). */
    static void on_deferred_call();
    
    /** Registered sigio callback from FileHandle. */    
    static void on_sigio();

private:
   
    /* Definition for decoder state machine. */
    typedef enum 
    {
        DECODER_STATE_SYNC = 0,
        DECODER_STATE_DECODE,
        DECODER_STATE_MAX
    } DecoderState;
    
    /* Definition for Tx state machine. */
    typedef enum 
    {
        TX_IDLE = 0,
        TX_RETRANSMIT_ENQUEUE,
        TX_RETRANSMIT_DONE,
        TX_INTERNAL_RESP,
        TX_STATE_MAX
    } TxState;
    
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
        FRAME_TX_TYPE_UA,
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
    
    /** Construct ua response message.
     */                
    static void ua_response_construct();
    
    /** Do write operation if pending data available.
     * @todo: document return code
     */
    static ssize_t write_do();
    
    /** Do read operation. */
    static void read_do();

    /** Encode a byte.
     * 
     *  @return Encoded byte.
     */        
    static uint8_t encode_do();
    
    /** Do a decode operation for a byte read. */
    static void decode_do();
    
    /** Frame decoder state machine operation. */    
    static void decoder_state_sync_run();
    
    /** Frame decoder state machine operation. */        
    static void decoder_state_decode_run();
    
    /** Change frame decoder state machine state. 
     * 
     *  @param new_state    State to transit.
     */            
    static void decoder_state_change(DecoderState new_state);
    
    /** Evaluate is rx frame valid for further processing. 
     * 
     *  @return true when frame is valid, false otherwise.
     */                
    static bool is_rx_frame_valid();
    
    /** Evaluate is suspending rx processing required.
     * 
     *  @return true when suspending is required, false otherwise.
     */                
    static bool is_rx_suspend_requited();
       
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
    
    /** UA frame tx path post processing. */        
    static void on_post_tx_frame_ua();
    
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
    
    /** State entry function. */
    static void tx_retransmit_done_entry_run();  
    static void tx_idle_entry_run();   
    typedef void (*tx_state_entry_func_t)();       
    /** Change frame decoder state machine state. 
     * 
     *  @param new_state    State to transit.
     *  @param entry_func   State entry function.
     */                
    static void tx_state_change(TxState new_state, tx_state_entry_func_t entry_func);
       
    // @todo: update me!
    static FileHandle * dlci_id_append(uint8_t dlci_id);
    static bool is_dlci_in_use(uint8_t dlci_id);
    static bool is_dlci_q_full();
    
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
       
    } tx_context_t;
       
    /* Definition for Rx context type. */
    typedef struct 
    {        
        DecoderState decoder_state;                  /* Decoder state machine current state. */
        uint8_t      offset;                         /* Offset in the buffer where to read to. */
        uint8_t      buffer[MBED_CONF_BUFFER_SIZE];  /* Rx buffer. */
       
    } rx_context_t;    
    
    /* Definition for state type. */
    typedef struct
    {
        uint8_t is_mux_open        : 1;         /* True when multiplexer is open. */       
        uint8_t is_initiator : 1;               /* True when role is initiator. */
        uint8_t is_mux_open_self_iniated_pending : 1;
        uint8_t is_mux_open_self_iniated_running : 1;
#if 0        
        uint8_t is_dlci_establish_pending : 1;  /* True if @ref mux_start or @ref dlci_establish is pending. */
        uint8_t is_user_tx_pending : 1;         /* True if user TX request is pending. */
        uint8_t is_internal_tx_pending : 1;     /* True if internal TX response is pending. */
#endif //         
    } state_t;
    
    static FileHandle      *_serial;                                /* Serial used. */        
    static MuxCallback     *_mux_obj_cb;                            /* Multiplexer object callback used. */  
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
};


/* Callback API for the Mux user. */
class MuxCallback {
public:

    /** Multiplexer control channel establishment event.
     *
     *  @note: This is only called for the peer initiated establishment. 
     *
     *  @param success  True upon establishment success, false in other case.
     */            
    virtual void on_mux_start(/*bool success DON*T CARE*/) = 0;
        
    /** DLCI establishment event.
     *
     *  @note: This is only called for the peer initiated establishment. 
     *
     *  @param obj      Valid object upon establishment success, NULL upon failure.     
     *  @param dlci_id  ID of the DLCI established. Range of 1 - 63.      
     */            
    virtual void on_dlci_establish(FileHandle *obj, uint8_t dlci_id) = 0;
    
    /** Generic event receive. 
     *
     *  @param event Event to receive.
     */
    virtual void event_receive(/*const Mux::mux_event_t &event*/) = 0;    
};

} // namespace mbed

#endif
