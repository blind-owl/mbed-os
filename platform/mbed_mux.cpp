
#include "mbed_mux.h"
#include "mbed_assert.h"
#include "EventQueueMock.h"

namespace mbed {

#define FLAG_SEQUENCE_OCTET                 0xF9u         /* Flag field used in the basic option mode. */    
#define ADDRESS_MUX_START_REQ_OCTET         0x03u         /* Address field of the start multiplexer request frame. */   
/* Address field value of the start multiplexer response frame. */        
#define ADDRESS_MUX_START_RESP_OCTET        ADDRESS_MUX_START_REQ_OCTET
#define FCS_INPUT_LEN                       3u            /* Length of the input for FCS calculation in number of 
                                                             bytes. Consisting of address, control and length fields. */
#define SABM_FRAME_LEN                      6u            /* Length of the SABM frame in number of bytes. */
#define UA_FRAME_LEN                        6u            /* Length of the UA frame in number of bytes. */
#define DM_FRAME_LEN                        6u            /* Length of the DM frame in number of bytes. */
#define UIH_FRAME_MIN_LEN                   6u            /* Minimum length of user frame. */
#define T1_TIMER_VALUE                      300u          /* T1 timer value. */
#define RETRANSMIT_COUNT                    3u            /* Retransmission count for the tx frames requiring a
                                                             response. */
#define FRAME_TYPE_SABM                     0x2Fu         /* SABM frame type coding in the frame control field. */
#define FRAME_TYPE_UA                       0x63u         /* UA frame type coding in the frame control field. */
#define FRAME_TYPE_DM                       0x0Fu         /* DM frame type coding in the frame control field. */
#define FRAME_TYPE_DISC                     0x43u         /* DISC frame type coding in the frame control field. */
#define FRAME_TYPE_UIH                      0xEFu         /* UIH frame type coding in the frame control field. */
#define PF_BIT                              (1u << 4)     /* P/F bit position in the frame control field. */
#define CR_BIT                              (1u << 1)     /* C/R bit position in the frame address field. */
#define DLCI_ID_LOWER_BOUND                 1u            /* Lower bound value of DLCI id.*/
#define DLCI_ID_UPPER_BOUND                 63u           /* Upper bound value of DLCI id.*/  
    
/* Definition for frame header type. */
typedef struct
{
    uint8_t flag_seq;       /* Flag sequence field. */
    uint8_t address;        /* Address field. */
    uint8_t control;        /* Control field. */
    uint8_t length;         /* Length field. */
    uint8_t information[1]; /* Begin of the information field if present. */
} frame_hdr_t;    

volatile uint8_t Mux::_establish_status = 0;
volatile uint8_t Mux::_dlci_id          = 0;
FileHandle *Mux::_serial                = NULL;
EventQueueMock *Mux::_event_q           = NULL;
MuxDataService Mux::_mux_objects[MBED_CONF_MUX_DLCI_COUNT];

//rtos::Semaphore Mux::_semaphore(0);
SemaphoreMock Mux::_semaphore;

Mux::tx_context_t Mux::_tx_context;
Mux::rx_context_t Mux::_rx_context;
Mux::state_t      Mux::_state;
const uint8_t     Mux::_crctable[MUX_CRC_TABLE_LEN] = {
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,  0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,  0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,  0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,  0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,

    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,  0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,  0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,  0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,  0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,

    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,  0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,  0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,  0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,  0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,

    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,  0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,  0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,  0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,  0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};

extern void trace(char *string, int data);

void Mux::module_init()
{
    _state.is_mux_open                       = 0;
    _state.is_mux_open_self_iniated_pending  = 0;
    _state.is_mux_open_self_iniated_running  = 0;    
    _state.is_dlci_open_self_iniated_pending = 0;
    _state.is_dlci_open_self_iniated_running = 0;          
    _state.is_write_error                    = 0;
    _state.is_user_thread_context            = 0;
    _state.is_tx_callback_context            = 0;
    _state.is_user_tx_pending                = 0;
   
    _rx_context.offset               = 0;
    _rx_context.frame_trailer_length = 0;        
    _rx_context.rx_state             = RX_FRAME_START;   
    
    _tx_context.tx_state            = TX_IDLE;    
    _tx_context.tx_callback_context = 0;
    
    _establish_status = static_cast<MuxEstablishStatus>(Mux::MUX_ESTABLISH_MAX);
    
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);
    for (uint8_t i = 0; i != end; ++i) {
        _mux_objects[i].dlci = MUX_DLCI_INVALID_ID;
    }    
}


void Mux::frame_retransmit_begin()
{   
//trace("Mux::frame_retransmit ", 0);    
    _tx_context.bytes_remaining = _tx_context.offset;
    _tx_context.offset          = 0;
}


void Mux::on_timeout()
{
//trace("on_timeout:_tx_context.tx_state", _tx_context.tx_state);    
// ADD STATE STUFF
//trace("Mux::on_timeout ", _tx_context.retransmit_counter);     
    switch (_tx_context.tx_state) {
        case TX_RETRANSMIT_DONE:
            if (_tx_context.retransmit_counter != 0) {
                --(_tx_context.retransmit_counter);
                frame_retransmit_begin();
                tx_state_change(TX_RETRANSMIT_ENQUEUE, tx_retransmit_enqueu_entry_run, NULL);
                if (_state.is_write_error) {
                    tx_state_change(TX_IDLE, tx_idle_entry_run, NULL); // @todo: untested code
                }
            } else {
                /* Retransmission limit reached, change state and release the suspended call thread with appropriate 
                   status code. */
                _establish_status        = MUX_ESTABLISH_TIMEOUT;
                const osStatus os_status = _semaphore.release();
                MBED_ASSERT(os_status == osOK);    
                tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);
                // @todo: need to be carefull of call order between the thread release and state change.
            }            
            break;
        default:
            /* No implementation required. */
            break;
    }
}


void Mux::dm_response_construct()
{
//    trace("dm_response_construct: ", 0);        
    
    // @todo: combine common functionality with other response function.
    
    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::_tx_context.buffer[0]));
    
    frame_hdr->flag_seq       = FLAG_SEQUENCE_OCTET;
    /* As multiplexer is not open we allways invert the C/R bit from the request frame. NOT!!*/
    frame_hdr->address        = _rx_context.buffer[1] /*^ CR_BIT*/;
    frame_hdr->control        = (FRAME_TYPE_DM | PF_BIT);    
    frame_hdr->length         = 1u;    
    frame_hdr->information[0] = fcs_calculate(&(Mux::_tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    // @todo: make XXX_FRAME_LEN define
    
    _tx_context.bytes_remaining = DM_FRAME_LEN;
    _tx_context.offset          = 0;               
}


void Mux::on_rx_frame_sabm()
{    
    /* No implementation required. */
}


void Mux::on_rx_frame_ua()
{
    // @todo: verify that we have issued the start/establishment request in the 1st place?
    // @todo: DEFECT we should do request-response DLCI ID matching

    switch (_tx_context.tx_state) {
        osStatus os_status;
        uint8_t dlci_id;
        case TX_RETRANSMIT_DONE:
            _event_q->cancel(_tx_context.timer_id);           
            _establish_status = Mux::MUX_ESTABLISH_SUCCESS;
            dlci_id           = _rx_context.buffer[1] >> 2;            
            if (dlci_id != 0) {
                dlci_id_append(dlci_id);
            } 
            os_status = _semaphore.release();
            MBED_ASSERT(os_status == osOK); 
            // @todo: need to verify correct call order for sm change and Semaphore release.
            tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);          
            break;
        default:
            /* Code that should never be reached. */
            // @todo: just silently ignore?
            trace("on_rx_frame_ua: ", _tx_context.tx_state);                
            MBED_ASSERT(false);
            break;
    }
}


void Mux::on_rx_frame_dm()
{
    // @todo: verify that we have issued the start request

    switch (_tx_context.tx_state) {
        osStatus os_status;
        case TX_RETRANSMIT_DONE:
            _event_q->cancel(_tx_context.timer_id);           
            _establish_status = Mux::MUX_ESTABLISH_REJECT;
            os_status = _semaphore.release();
            MBED_ASSERT(os_status == osOK);        
            // @todo: need to verify correct call order for sm change and Semaphore release.
            tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);            
            break;
        default:
            trace("on_rx_frame_dm: ", _tx_context.tx_state);                
            MBED_ASSERT(false);
            break;
    }
}

// @todo: DEFECT FRAME POST TX NEEDS TO BE CALLED ALLWAYS WHEN TX FRAME DONE NOW JUST IN on_deferredd_call
void Mux::tx_internal_resp_entry_run()
{
    write_do();
}


void Mux::dm_response_send()
{
    dm_response_construct();
    
    tx_state_change(TX_INTERNAL_RESP, tx_internal_resp_entry_run, tx_idle_exit_run);      
    // @todo DEFECT we should check write error bit and transit back to TX_IDLE - NOT? DO IT IN THE ERR DETECT PLACE
}


void Mux::on_rx_frame_disc()
{
    const uint8_t dlci_id = _rx_context.buffer[1] >> 2;    
   
    switch (_tx_context.tx_state) {
        case TX_IDLE:                      
            if (!_state.is_mux_open) {
                dm_response_send();
            } else {
                if (dlci_id != 0) {
                    if (!is_dlci_in_use(dlci_id)) {
                        dm_response_send();
                    } else {
                        /* DLCI close not supported and silently discarded. */
                    }
                } else {
                    /* Mux close not supported and silently discarded. */
                }
            }
            break;
        default:
            /* @todo: DEFECT implement missing functionality. */
            trace("on_rx_frame_disc: ", _tx_context.tx_state);              
            MBED_ASSERT(false);       
            break;            
    }
}


void Mux::on_rx_frame_uih()
{
    MBED_ASSERT(false);        
}


void Mux::on_rx_frame_not_supported()
{
    // @todo: test me
    trace("rx_frame_not_supported_do: ", _rx_context.buffer[2]);        
    
    MBED_ASSERT(false);    
}
    

Mux::FrameRxType Mux::frame_rx_type_resolve()
{
//trace("frame_type_resolve ", _rx_context.buffer[2]);    

    const uint8_t frame_type = (_rx_context.buffer[2] & ~PF_BIT);
    
    if (frame_type == FRAME_TYPE_SABM) {
        return FRAME_RX_TYPE_SABM;
    } else if (frame_type == FRAME_TYPE_UA) {
        return FRAME_RX_TYPE_UA;
    } else if (frame_type == FRAME_TYPE_DM) {
        return FRAME_RX_TYPE_DM;
    } else if (frame_type == FRAME_TYPE_DISC) {
        return FRAME_RX_TYPE_DISC;
    } else if (frame_type == FRAME_TYPE_UIH) {
        return FRAME_RX_TYPE_UIH;
    } else {
        return FRAME_RX_TYPE_NOT_SUPPORTED;
    }
}


void Mux::tx_state_change(TxState new_state, tx_state_entry_func_t entry_func, tx_state_exit_func_t exit_func)
{
    if (exit_func != NULL) {
        exit_func();
    }

    _tx_context.tx_state = new_state;
    
//    trace("tx_state_change: ", _tx_context.tx_state);

    if (entry_func != NULL) {
        entry_func();
    }
}


void Mux::tx_retransmit_done_entry_run()
{
    _tx_context.timer_id = _event_q->call_in(T1_TIMER_VALUE, Mux::on_timeout);
    MBED_ASSERT(_tx_context.timer_id != 0);                
}
 

bool Mux::is_dlci_in_use(uint8_t dlci_id)
{  
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);
    for (uint8_t i = 0; i != end; ++i) {   
        if (_mux_objects[i].dlci == dlci_id) {
            return true;
        }        
    }
    
    return false;
}


void Mux::on_post_tx_frame_sabm()
{
    switch (_tx_context.tx_state) {
        case TX_RETRANSMIT_ENQUEUE:
            tx_state_change(TX_RETRANSMIT_DONE, tx_retransmit_done_entry_run, NULL);
            break;
        default:
            /* Code that should never be reached. */
            trace("_tx_context.tx_state", _tx_context.tx_state);                       
            MBED_ASSERT(false);
            break;
    }       
}


void Mux::pending_self_iniated_mux_open_start()
{
    /* Construct the frame, start the tx sequence, set and reset relevant state contexts. */
    _state.is_mux_open_self_iniated_running = 1u;       
    _state.is_mux_open_self_iniated_pending = 0;

    sabm_request_construct(0);
    tx_state_change(TX_RETRANSMIT_ENQUEUE, tx_retransmit_enqueu_entry_run, tx_idle_exit_run);
    if (!_state.is_write_error) {
        _tx_context.retransmit_counter = RETRANSMIT_COUNT;           
    } else {
        _establish_status        = MUX_ESTABLISH_WRITE_ERROR;
        const osStatus os_status = _semaphore.release();
        MBED_ASSERT(os_status == osOK);        
        tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);        
    }
}


void Mux::pending_self_iniated_dlci_open_start()
{
    /* Construct the frame, start the tx sequence, set and reset relevant state contexts. */    
    _state.is_dlci_open_self_iniated_running = 1u;
    _state.is_dlci_open_self_iniated_pending = 0;

    sabm_request_construct(_dlci_id);
    tx_state_change(TX_RETRANSMIT_ENQUEUE, tx_retransmit_enqueu_entry_run, tx_idle_exit_run);
    if (!_state.is_write_error) {
        _tx_context.retransmit_counter = RETRANSMIT_COUNT;           
    } else {
        _establish_status        = MUX_ESTABLISH_WRITE_ERROR;
        const osStatus os_status = _semaphore.release();
        MBED_ASSERT(os_status == osOK);        
        tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);        
    }   
}


void Mux::tx_idle_exit_run()
{
    _state.is_write_error = 0;
}


uint8_t Mux::tx_callback_index_advance()
{
    /* Increment and get the index bit accounting the roll over. */
    uint8_t index = ((_tx_context.tx_callback_context & 0xF0u) >> 4);
    index <<= 1;
    if ((index & 0x0Fu) == 0) {
        index = 1u;
    }

    /* Clear existing index bit and assign the new incremented index bit. */
    _tx_context.tx_callback_context &= 0x0Fu;
    _tx_context.tx_callback_context |= (index << 4);
    
    return index;
}


uint8_t Mux::tx_callback_pending_mask_get()
{
    return (_tx_context.tx_callback_context & 0x0Fu);
}


void Mux::tx_callback_pending_bit_clear(uint8_t bit)
{
    _tx_context.tx_callback_context &= ~bit;
}


void Mux::tx_callback_pending_bit_set(uint8_t dlci_id)
{  
    uint8_t i         = 0;
    uint8_t bit       = 1u;    
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);    

    do {
        if (_mux_objects[i].dlci == dlci_id) {
            break;
        }
        
        ++i;
        bit <<= 1;
    } while (i != end);
    
    MBED_ASSERT(i != end);    
    
    _tx_context.tx_callback_context |= bit;
    
//trace("!!tx_callback_pending_bit_set", bit);    
}


MuxDataService& Mux::tx_callback_lookup(uint8_t bit)
{
    uint8_t i         = 0;    
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);

    do {        
        bit >>= 1;
        if (bit == 0) {
            break;
        }
        
        ++i;
    } while (i != end);
    
    MBED_ASSERT(i != end);
    
    return _mux_objects[i];
}


void Mux::tx_callback_dispatch(uint8_t bit)
{
    MuxDataService& obj = tx_callback_lookup(bit);
    obj._sigio_cb();
}


void Mux::tx_idle_entry_run()
{
#if 0    
trace("tx_idle_entry_run: ", _state.is_dlci_open_peer_iniated_pending);
#endif // 0

    if (_state.is_mux_open_self_iniated_pending) {
        pending_self_iniated_mux_open_start();
    } else if (_state.is_dlci_open_self_iniated_pending) {
        pending_self_iniated_dlci_open_start();
    } else {
        /* No implementation required. */
    }

//@todo: SHOULD TX CALLBACK DISPATCH BE IN else clause above or even the first one??

    /* TX callback processing block below could be entered recursively within same thread context. Protection bit is 
     * used to prevent that. */
    if (!_state.is_tx_callback_context) {
        /* Lock this code block from multiple entries within same thread context by using a protection bit. Check and 
           process possible pending user TX callback request as long there is something pending. Round robin 
           shceduling used for dispatching pending TX callback in order to prevent starvation of pending callbacks. */
        
        _state.is_tx_callback_context = 1u;
      
        uint8_t current_tx_index;              
        uint8_t tx_callback_pending_mask = tx_callback_pending_mask_get();        
//trace("E_LOOP_2", tx_callback_pending_mask);                                       
        while (tx_callback_pending_mask != 0) {
            
            /* Locate 1st pending TX callback. */            
            do {
                current_tx_index = tx_callback_index_advance();
            } while ((current_tx_index & tx_callback_pending_mask) == 0);
            
            /* Clear pending bit and dispatch TX callback. */
            tx_callback_pending_bit_clear(current_tx_index);
//trace("TX_PEND-DISPATCH", current_tx_index);                   
            tx_callback_dispatch(current_tx_index);
            
            /* No valid use case exists for TX cycle activation within TX callback as per design user TX is started 
               within this while loop using @ref is_user_tx_pending bit and per system design DLCI establishment is not 
               allowed within callback context as this would leave to system thread context dead lock as DLCI 
               establishment includes semaphore wait. We will enforce this by MBED_ASSERT below. Note that @ref 
               dlci_establish will have MBED_ASSERT to enforce calling that API from system thread context thus the 
               assert below is not absolutely necessary. */
            MBED_ASSERT(_tx_context.tx_state == TX_IDLE);
                                          
            if (_state.is_user_tx_pending) {
                /* User TX was requested within callback context dispatched above. */
//trace("TX_PEND-RUN", 0);                                           
                /* TX buffer was constructed within @ref user_data_tx, now start the TX cycle. */
                _state.is_user_tx_pending = 0;                                             
                tx_state_change(TX_NORETRANSMIT, tx_noretransmit_entry_run, tx_idle_exit_run);
                if (_tx_context.tx_state != TX_IDLE) {
                    /* TX cycle not finished within current context, stop callback processing as we will continue when 
                       TX cycle finishes and this function is re-entered. */ 
                            
                    break;
                }
            }

            tx_callback_pending_mask = tx_callback_pending_mask_get();            
        }
        
        _state.is_tx_callback_context = 0;        
    }
}
 
 
void Mux::on_post_tx_frame_dm()
{
    switch (_tx_context.tx_state) {
        case TX_INTERNAL_RESP:   
            tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);           
            break;
        default:
            /* Code that should never be reached. */
            trace("_tx_context.tx_state", _tx_context.tx_state);                       
            MBED_ASSERT(false);
            break;
    }                   
}


void Mux::on_post_tx_frame_uih()
{
    switch (_tx_context.tx_state) {
        case TX_NORETRANSMIT:   
            tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);           
            break;
        default:
            /* Code that should never be reached. */
            trace("_tx_context.tx_state", _tx_context.tx_state);                       
            MBED_ASSERT(false);
            break;
    }                   
}


Mux::FrameTxType Mux::frame_tx_type_resolve()
{
    const uint8_t frame_type = (_tx_context.buffer[2] & ~PF_BIT);
    
    if (frame_type == FRAME_TYPE_SABM) {
        return FRAME_TX_TYPE_SABM;
    } else if (frame_type == FRAME_TYPE_DM) {
        return FRAME_TX_TYPE_DM;
    } else if (frame_type == FRAME_TYPE_UIH) {
        return FRAME_TX_TYPE_UIH;
    } else {
        trace("frame_type: ", frame_type);                               
        MBED_ASSERT(false);
        return FRAME_TX_TYPE_MAX;
    }
}


#define FRAME_START_READ_LEN 1u    
ssize_t Mux::on_rx_read_state_frame_start()
{
//trace("!RX-FRAME_START", 0);

    ssize_t read_err;
    _rx_context.buffer[_rx_context.offset] = static_cast<uint8_t>(~FLAG_SEQUENCE_OCTET);
    do {
        read_err = _serial->read(&(_rx_context.buffer[_rx_context.offset]), FRAME_START_READ_LEN);
//trace("!READ_VALUE", _rx_context.buffer[_rx_context.offset]);        
    } while ((_rx_context.buffer[_rx_context.offset] != FLAG_SEQUENCE_OCTET) && (read_err != -EAGAIN));
    
    if (_rx_context.buffer[_rx_context.offset] == FLAG_SEQUENCE_OCTET) {        
        _rx_context.offset  += FRAME_START_READ_LEN;
        _rx_context.rx_state = RX_HEADER_READ;
    }

//trace("!read_err", read_err);    
    return read_err;
}

#define FRAME_HEADER_READ_LEN 3u
#define FRAME_TRAILER_LEN     2u
ssize_t Mux::on_rx_read_state_header_read()
{
//trace("!RX-FRAME_HEADER", 0);

    ssize_t read_err;
    // @todo:DEFECT we need counter for this(as we can enter this func multiple times) = > reuse frame_trailer_length? 
    // Set init value in state transit time.
    size_t read_len = FRAME_HEADER_READ_LEN;
    do {
//trace("read_len", read_len);
        read_err = _serial->read(&(_rx_context.buffer[_rx_context.offset]), read_len);
        if (read_err != -EAGAIN) {
            if ((_rx_context.offset == 1u) && (_rx_context.buffer[_rx_context.offset] == FLAG_SEQUENCE_OCTET)) {
                /* Overlapping block move 1-index possible trailing data after the flag sequence octet. */
//trace("!!SKIP!!", 0);                
                memmove(&(_rx_context.buffer[_rx_context.offset]), 
                        &(_rx_context.buffer[_rx_context.offset + 1u]),
                        (read_err - 1u));
                
                _rx_context.offset += (read_err - 1u);
                read_len           -= (read_err - 1u);                

                //@todo: proper TCs needed for this branch??
            } else {
                _rx_context.offset += read_err;
                read_len           -= read_err;                
            }
        }
    } while ((read_len != 0) && (read_err != -EAGAIN));
    
    if (_rx_context.offset == (FRAME_HEADER_READ_LEN + FRAME_START_READ_LEN)) {       
        /* Decode remaining frame read length and change state. Current implementation supports only 1-byte length 
           field, enforce this with MBED_ASSERT. */

        MBED_ASSERT((_rx_context.buffer[_rx_context.offset - 1u] & 1u) == 1u);
        _rx_context.frame_trailer_length = (_rx_context.buffer[_rx_context.offset - 1u] & ~1u) + FRAME_TRAILER_LEN;
//trace("_rx_context.frame_trailer_length", _rx_context.frame_trailer_length);        
        _rx_context.rx_state             = RX_TRAILER_READ;
    }

    return read_err;
}


ssize_t Mux::on_rx_read_state_trailer_read()
{
//trace("!RX-FRAME_TRAILER", 0);

    typedef void (*rx_frame_decoder_func_t)();    
    static const rx_frame_decoder_func_t rx_frame_decoder_func[FRAME_RX_TYPE_MAX] = {
        on_rx_frame_sabm,
        on_rx_frame_ua,
        on_rx_frame_dm,
        on_rx_frame_disc,
        on_rx_frame_uih,
        on_rx_frame_not_supported
    };
    
    ssize_t read_err;
    do {
        read_err = _serial->read(&(_rx_context.buffer[_rx_context.offset]), _rx_context.frame_trailer_length);
        if (read_err != -EAGAIN) {
            _rx_context.frame_trailer_length -= read_err;
        }
    } while ((_rx_context.frame_trailer_length != 0) && (read_err != -EAGAIN));
    
    if (_rx_context.frame_trailer_length == 0) {
        /* Complete frame received, decode frame type and process it. */

        const Mux::FrameRxType        frame_type = frame_rx_type_resolve();
        const rx_frame_decoder_func_t func       = rx_frame_decoder_func[frame_type];
        func();      
   
        _rx_context.offset   = 1u;            // @todo: only set when travel back to RX_HEADER_READ
        _rx_context.rx_state = RX_HEADER_READ;        
        
        read_err = -EAGAIN;
    }
    
    return read_err;
}


ssize_t Mux::on_rx_read_state_suspend()
{
    MBED_ASSERT(false);
    
    return -EAGAIN;
}


void Mux::rx_event_do(RxEvent event)    
{
    typedef ssize_t (*rx_read_func_t)();    
    static const rx_read_func_t rx_read_func[RX_STATE_MAX] = {
        on_rx_read_state_frame_start,
        on_rx_read_state_header_read,
        on_rx_read_state_trailer_read,
        on_rx_read_state_suspend,
    };
      
    switch (event) {
        ssize_t        read_err;
        rx_read_func_t func;
        case RX_READ:            
            do {
                func     = rx_read_func[_rx_context.rx_state];
//trace("!RUN-READ", 0);                
                read_err = func();
            } while (read_err != -EAGAIN);
            
            break;
        case RX_RESUME:
            if (_rx_context.rx_state == RX_SUSPEND) {
                _rx_context.rx_state = RX_HEADER_READ;
                
                // @todo: schedule system thread
                MBED_ASSERT(false);
            } else {
                /* No implementation required. */
            }
            
            break;
        default:
            /* Code that should never be reached. */
            MBED_ASSERT(false);
            break;
    }
}


void Mux::write_do()
{
    switch (_tx_context.tx_state) {
        ssize_t write_err;
        case TX_NORETRANSMIT:
        case TX_RETRANSMIT_ENQUEUE:
        case TX_INTERNAL_RESP:    
//trace("!B-bytes_remaining: ", _tx_context.bytes_remaining);
            write_err = 1;
//            while ((_tx_context.bytes_remaining != 0) && (write_err > 0)) { // @todo: change to do...while
            do {
                write_err = _serial->write(&(_tx_context.buffer[_tx_context.offset]), _tx_context.bytes_remaining);
                if (write_err > 0) {
                    _tx_context.bytes_remaining -= write_err;
                    _tx_context.offset          += write_err;
                }  
//trace("!write_err: ", write_err);        
            } while ((_tx_context.bytes_remaining != 0) && (write_err > 0));
//trace("!E-bytes_remaining: ", _tx_context.bytes_remaining);    
            if (_tx_context.bytes_remaining == 0) {
                /* Frame write complete, execute correct post processing function for clean-up. */
                
                typedef void (*post_tx_frame_func_t)();
                static const post_tx_frame_func_t post_tx_func[FRAME_TX_TYPE_MAX] = {
                    on_post_tx_frame_sabm,
                    on_post_tx_frame_dm,
                    on_post_tx_frame_uih
                };
                const Mux::FrameTxType     frame_type = frame_tx_type_resolve();
                const post_tx_frame_func_t func       = post_tx_func[frame_type];
                func();                                
            } else if (write_err < 0) {
                switch (_tx_context.tx_state) {
                    osStatus os_status;
                    case TX_RETRANSMIT_ENQUEUE:
                        if (_state.is_user_thread_context) {
                            _state.is_write_error = 1u;
                        } else {
                            _establish_status = MUX_ESTABLISH_WRITE_ERROR;
                            os_status         = _semaphore.release();
                            MBED_ASSERT(os_status == osOK);                              
                        }
                        
                        tx_state_change(TX_IDLE, tx_idle_entry_run, NULL);            
                        break;
                    default:
                        // @todo: write failure for non user orgined TX: propagate error event to the user
                        MBED_ASSERT(false);
                        break;
                }                  
            } else {
                /* No implementation required. */
            }
            break;
        default:
            /* No implementattion required. */
            break;
    }
}


void Mux::on_deferred_call()
{   
    rx_event_do(RX_READ);
    write_do();
}


void Mux::on_sigio()
{
    const int id = _event_q->call(Mux::on_deferred_call);
    MBED_ASSERT(id != 0);
}


void Mux::eventqueue_attach(EventQueueMock *event_queue)
{
    _event_q = event_queue;
}


void Mux::serial_attach(FileHandle *serial)
{
    _serial = serial;
    
    _serial->sigio(Mux::on_sigio);
}


uint8_t Mux::fcs_calculate(const uint8_t *buffer,  uint8_t input_len)
{
    uint8_t fcs = 0xFFu;

    while (input_len-- != 0) {
        fcs = _crctable[fcs^*buffer++];
    }
    
    /* Ones complement. */
    fcs = 0xFFu - fcs;
 
//    trace("FCS: ", fcs);

    return fcs;
}


void Mux::sabm_request_construct(uint8_t dlci_id)
{
    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::_tx_context.buffer[0]));
    
    frame_hdr->flag_seq = FLAG_SEQUENCE_OCTET;
    if (dlci_id == 0) {
        frame_hdr->address = 3 | (dlci_id << 2);                  
    } else {
        frame_hdr->address = (/*_state.is_initiator ? */3/* : 1*/) | (dlci_id << 2);                  
    }
    frame_hdr->control        = (FRAME_TYPE_SABM | PF_BIT);         
    frame_hdr->length         = 1u;
    frame_hdr->information[0] = fcs_calculate(&(Mux::_tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    _tx_context.bytes_remaining = SABM_FRAME_LEN;
    _tx_context.offset          = 0;                
}


bool Mux::is_dlci_q_full()
{
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);
    for (uint8_t i = 0; i != end; ++i) {  
        if (_mux_objects[i].dlci == MUX_DLCI_INVALID_ID) {     
            return false;
        }
    }
    
    return true;
}


void Mux::dlci_id_append(uint8_t dlci_id)
{
    uint8_t i         = 0;
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);    
    do {
        if (_mux_objects[i].dlci == MUX_DLCI_INVALID_ID) {
            _mux_objects[i].dlci = dlci_id;
            
            break;
        }
        
        ++i;
    } while (i != end);
    
    /* Internal implementation error if append was not done as Q size is checked prior call. */
    MBED_ASSERT(i != end);
}


FileHandle * Mux::file_handle_get(uint8_t dlci_id)
{
    FileHandle *obj   = NULL;
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);
    for (uint8_t i = 0; i != end; ++i) {   
        if (_mux_objects[i].dlci == dlci_id) {
            obj = &(_mux_objects[i]);
            
            break;
        }
    }
       
    return obj;
}
    
    
uint32_t Mux::dlci_establish(uint8_t dlci_id, MuxEstablishStatus &status, FileHandle **obj)
{       
    if ((dlci_id < DLCI_ID_LOWER_BOUND) || (dlci_id > DLCI_ID_UPPER_BOUND)) {
        return 2u;
    }
// @todo: add mutex_lock    
    if (!_state.is_mux_open) {
// @todo: add mutex_free                
        return 1u;
    }
    if (is_dlci_q_full()) {
// @todo: add mutex_free                        
        return 0;
    }
    if (is_dlci_in_use(dlci_id)) {
// @todo: add mutex_free                        
        return 0;
    }
    if (_state.is_dlci_open_self_iniated_pending) {
// @todo: add mutex_free                        
        return 3u;        
    }
    if (_state.is_dlci_open_self_iniated_running) {
// @todo: add mutex_free                        
        return 3u;                
    }
    
    switch (_tx_context.tx_state) {
        int              ret_wait;
        case TX_IDLE:
            /* Construct the frame, start the tx sequence, and suspend the call thread upon write sequence success. */  
            sabm_request_construct(dlci_id);
            _tx_context.retransmit_counter = RETRANSMIT_COUNT; // @todo: set to tx_idle_exit           
            tx_state_change(TX_RETRANSMIT_ENQUEUE, tx_retransmit_enqueu_entry_run, tx_idle_exit_run);
            if (_state.is_write_error) {
                status = MUX_ESTABLISH_WRITE_ERROR;
// @todo: add mutex_free                                
                return 4u;
            }               

            _state.is_dlci_open_self_iniated_running = 1u;
              
// @todo: add mutex_free here               
            ret_wait = _semaphore.wait();
            MBED_ASSERT(ret_wait == 1);
            status = static_cast<MuxEstablishStatus>(_establish_status);
            if (status == MUX_ESTABLISH_SUCCESS) {
                *obj = file_handle_get(dlci_id);
                MBED_ASSERT(*obj != NULL);
            }
            break;
        case TX_INTERNAL_RESP:
        case TX_NORETRANSMIT:
            _state.is_dlci_open_self_iniated_pending = 1u;
            _dlci_id                                 = dlci_id;
// @todo: add mutex_free               
            ret_wait = _semaphore.wait();
            MBED_ASSERT(ret_wait == 1);        
            status = static_cast<MuxEstablishStatus>(_establish_status);
            switch (status) {
                case MUX_ESTABLISH_SUCCESS:
                    *obj = file_handle_get(dlci_id);
                    MBED_ASSERT(*obj != NULL);                    
                    break;
                case MUX_ESTABLISH_MAX:
                    /* DLCI ID is allready in use so self iniated DLCI establishment was not started, it is ok to 
                       exit directly from here with appropriate error code.*/
                    return 0;
                default:
                    /* No implementation required. */
                    break;
            }
            break;            
        default:
            MBED_ASSERT(false);
            break;
    }    
    
    _state.is_dlci_open_self_iniated_running = 0;            

    return 4u;
}


void Mux::tx_retransmit_enqueu_entry_run()
{
    _state.is_user_thread_context = 1u;
    write_do();
    _state.is_user_thread_context = 0;
}

    
uint32_t Mux::mux_start(Mux::MuxEstablishStatus &status)
{
// @todo: add mutex_lock
       
    if (_state.is_mux_open) { 
// @todo: add mutex_free        
        return 0;
    }
    if (_state.is_mux_open_self_iniated_pending) { 
// @todo: add mutex_free                
        return 1u;
    }
    if (_state.is_mux_open_self_iniated_running) {
// @todo: add mutex_free                
        return 1u;        
    }

    switch (_tx_context.tx_state) {
        int              ret_wait;
        case TX_IDLE:
            /* Construct the frame, start the tx sequence, and suspend the call thread upon write sequence success. */
            sabm_request_construct(0);
            _tx_context.retransmit_counter = RETRANSMIT_COUNT;            
            tx_state_change(TX_RETRANSMIT_ENQUEUE, tx_retransmit_enqueu_entry_run, tx_idle_exit_run);
            if (_state.is_write_error) {
                status = MUX_ESTABLISH_WRITE_ERROR;
// @todo: add mutex_free                                
                return 2u;
            }               

            _state.is_mux_open_self_iniated_running = 1u;
              
// @todo: add mutex_free here               
            ret_wait = _semaphore.wait();
            MBED_ASSERT(ret_wait == 1);
            status = static_cast<MuxEstablishStatus>(_establish_status);
            if (status == MUX_ESTABLISH_SUCCESS) {
                _state.is_mux_open = 1u;                
            }                                         
            break;
        case TX_INTERNAL_RESP:
            _state.is_mux_open_self_iniated_pending = 1u;
// @todo: add mutex_free               
            ret_wait = _semaphore.wait();
            MBED_ASSERT(ret_wait == 1);        
            status = static_cast<MuxEstablishStatus>(_establish_status);
            if (status == MUX_ESTABLISH_SUCCESS) {
                _state.is_mux_open = 1u;                
            }
            break;
        default:
            /* Code that should never be reached. */
            trace("tx_state: ", _tx_context.tx_state);
            MBED_ASSERT(false);
            break;
    };
                
    _state.is_mux_open_self_iniated_running = 0;
   
    return 2u;   
}


void Mux::user_information_construct(uint8_t dlci_id, const void* buffer, size_t size)
{
    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::_tx_context.buffer[0]));
    
    frame_hdr->flag_seq = FLAG_SEQUENCE_OCTET;
    frame_hdr->address  = (/*_state.is_initiator ? */3u/* : 1u*/) | (dlci_id << 2);                 
    frame_hdr->control  = (FRAME_TYPE_UIH | PF_BIT);           
    frame_hdr->length   = (1u | (size << 1));
    
    memmove(&(frame_hdr->information[0]), buffer, size);
    
    uint8_t* fcs_pos = (&(frame_hdr->information[0]) + size);
    *fcs_pos         = fcs_calculate(&(Mux::_tx_context.buffer[1]), FCS_INPUT_LEN);    
    *(++fcs_pos)     = FLAG_SEQUENCE_OCTET;
    
    _tx_context.bytes_remaining = UIH_FRAME_MIN_LEN + size;
    _tx_context.offset          = 0;
}


void Mux::tx_noretransmit_entry_run()
{
//    _state.is_user_thread_context = 1u; // @todo: is this really so?? we should set this allways in dfc or timeout 
                                          // to be sure and use is_system_thread_context => FEELS LIKE A GOOD IDEA
    write_do();
//    _state.is_user_thread_context = 0;
}


ssize_t Mux::user_data_tx(uint8_t dlci_id, const void* buffer, size_t size)
{
// @todo: get mutex

    // @todo: add MBED_ASSERT for max size
    MBED_ASSERT(size <= (MBED_CONF_BUFFER_SIZE - 6u)); // @todo: define magic
    if (size != 0) {
        MBED_ASSERT(buffer != NULL);
    }
    
    ssize_t write_ret;
    switch (_tx_context.tx_state) {        
        case TX_IDLE:
            if (!_state.is_tx_callback_context) {
                /* Proper state to start TX cycle within current context. */
                
                user_information_construct(dlci_id, buffer, size);
                tx_state_change(TX_NORETRANSMIT, tx_noretransmit_entry_run, tx_idle_exit_run);
                
                write_ret = size;
            } else {
                /* Current context is TX callback context. */
                
                if (!_state.is_user_tx_pending) {
                    /* Signal callback context to start TX cycle and construct the frame. */
                    
                    /* @note: If user is starting a TX to a DLCI, which is after the current DLCI TX callback within the
                     * stored sequence this will result to dispatching 1 unnecessary TX callback, if this is a issue one
                     * should clear the TX callback pending bit marker for this DLCI in this place. */

//trace("TX_PEND-SET", 0);                    
                    _state.is_user_tx_pending = 1u;                    
                    user_information_construct(dlci_id, buffer, size);
                    
                    write_ret = size;
                } else {                    
                    /* TX cycle allready scheduled, set TX callback pending and inform caller by return value that no 
                       action was taken. */

                    tx_callback_pending_bit_set(dlci_id);
                    write_ret = 0;
                }
            }
            
            break;            
        default:
            /* TX allready in use, set TX callback pending and inform caller by return value that no action was taken. 
             */
            tx_callback_pending_bit_set(dlci_id);
            write_ret = 0;
            
            break;
    }
        
// @todo: release mutex   

    return write_ret;
}


} // namespace mbed
