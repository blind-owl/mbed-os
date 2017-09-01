
#include "mbed_mux.h"
#include "mbed_assert.h"
#include "EventQueueMock.h"

namespace mbed {

#define FLAG_SEQUENCE_OCTET                 0x7Eu         /* Flag field used in the advanced option mode. */
#define ADDRESS_MUX_START_REQ_OCTET         0x03u         /* Address field of the start multiplexer request frame. */   
/* Address field value of the start multiplexer response frame. */        
#define ADDRESS_MUX_START_RESP_OCTET        ADDRESS_MUX_START_REQ_OCTET
#define FCS_INPUT_LEN                       2u            /* Length of the input for FCS calculation in number of 
                                                             bytes. */
#define CONTROL_ESCAPE_OCTET                0x7Du         /* Control escape octet used as the transparency 
                                                             identifier. */
#define WRITE_LEN                           1u            /* Length of write in number of bytes. */    
#define READ_LEN                            1u            /* Length of read in number of bytes. */   
#define SABM_FRAME_LEN                      5u            /* Length of the SABM frame in number of bytes. */
#define UA_FRAME_LEN                        5u            /* Length of the UA frame in number of bytes. */
#define DM_FRAME_LEN                        5u            /* Length of the DM frame in number of bytes. */
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
    uint8_t information[1]; /* Begin of the information field if present. */
} frame_hdr_t;    

volatile uint8_t Mux::_establish_status = 0;
volatile uint8_t Mux::_dlci_id          = 0;
uint8_t          Mux::_address_field    = 0;
FileHandle *Mux::_serial                = NULL;
EventQueueMock *Mux::_event_q           = NULL;
MuxCallback *Mux::_mux_obj_cb           = NULL;
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
    _state.is_initiator                      = 0;
    _state.is_mux_open_self_iniated_pending  = 0;
    _state.is_mux_open_self_iniated_running  = 0;    
    _state.is_dlci_open_self_iniated_pending = 0;
    _state.is_dlci_open_self_iniated_running = 0;           
    _state.is_dlci_open_peer_iniated_pending = 0;
    _state.is_dlci_open_peer_iniated_running = 0;                    
   
    _rx_context.offset        = 0;
    _rx_context.decoder_state = DECODER_STATE_SYNC;    
    
    _tx_context.tx_state = TX_IDLE;    
    
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
   
    const ssize_t ret_write = write_do();   
    if (ret_write < 0) {
        MBED_ASSERT(false); // @todo propagate error to user.
    }       
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
                tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
            } else {
                /* Retransmission limit reached, change state and release the suspended call thread with appropriate 
                   status code. */
                _establish_status        = MUX_ESTABLISH_TIMEOUT;
                const osStatus os_status = _semaphore.release();
                MBED_ASSERT(os_status == osOK);    
                tx_state_change(TX_IDLE, tx_idle_entry_run);
                // @todo: need to be carefull of call order between the thread release and state change.
            }            
            break;
        default:
            /* No implementation required. */
            break;
    }
}


void Mux::decoder_state_change(DecoderState new_state)
{
    _rx_context.decoder_state = new_state;
}


void Mux::decoder_state_sync_run()
{
//trace("decoder_state_sync_run: ", _rx_context.buffer[_rx_context.offset]);    

    if (_rx_context.buffer[_rx_context.offset] == FLAG_SEQUENCE_OCTET) {
        ++_rx_context.offset;
        decoder_state_change(DECODER_STATE_DECODE);
    }
}


bool Mux::is_rx_frame_valid()
{
//    trace("is_rx_frame_valid::_rx_context.offset: ", _rx_context.offset);    
    
    return (_rx_context.offset >= 4) ? true : false;
}


bool Mux::is_rx_suspend_requited()
{
    // @todo: implement me!    
    return false;
}


void Mux::ua_response_construct(uint8_t dlci_id)
{
//trace("ua_response_construct: ", 0);        

    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::_tx_context.buffer[0]));
    
    frame_hdr->flag_seq       = FLAG_SEQUENCE_OCTET;
    frame_hdr->address        = (_state.is_initiator ? 1u : 3u) | (dlci_id << 2);
    frame_hdr->control        = (FRAME_TYPE_UA | PF_BIT);    
    frame_hdr->information[0] = fcs_calculate(&(Mux::_tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    // @todo: make START_RESP_FRAME_LEN define
    
    _tx_context.bytes_remaining = UA_FRAME_LEN;
    _tx_context.offset          = 0;               
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
    frame_hdr->information[0] = fcs_calculate(&(Mux::_tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    // @todo: make XXX_FRAME_LEN define
    
    _tx_context.bytes_remaining = DM_FRAME_LEN;
    _tx_context.offset          = 0;               
}


void Mux::on_rx_frame_sabm()
{    
    // @todo: DEFECT q checking functionality must be tx state agnostic in this function.
    
    switch (_tx_context.tx_state) {
        ssize_t return_code;
        uint8_t dlci_id;
        case TX_IDLE:
            /* Construct the frame, start the tx sequence 1-byte at time, reset relevant state contexts. */             
            
            // @todo: make decoder func for dlci_id;
            // @todo: verify dlci_id: not in use allready, use bitmap for it.
            dlci_id = _rx_context.buffer[1] >> 2;            
            if ((dlci_id == 0) || 
                ((dlci_id != 0) && _state.is_mux_open)) {
                if (is_dlci_in_use(dlci_id)) {                                   
                    ua_response_construct(dlci_id);
                } else {
                    if (!is_dlci_q_full()) {
                        ua_response_construct(dlci_id);
                    } else {
                        dm_response_construct();
                    }                        
                }
            } else {
                dm_response_construct();
            }
            
            return_code = write_do();    
            MBED_ASSERT(return_code != 0);
            if (return_code >= 0) {
                tx_state_change(TX_INTERNAL_RESP, NULL);
            } else {
                // @todo: propagate write error to user.
                MBED_ASSERT(false);
            }
            break;
        case TX_RETRANSMIT_ENQUEUE:
        case TX_RETRANSMIT_DONE:              
            dlci_id = _rx_context.buffer[1] >> 2;            
            if (!is_dlci_in_use(dlci_id)) {
                _address_field                           = _rx_context.buffer[1];
                _state.is_dlci_open_peer_iniated_pending = 1u;
            }
            break;
        default:
            /* Code that should never be reached. */
            trace("on_rx_frame_sabm: ", _tx_context.tx_state);    
            MBED_ASSERT(false);
            break;        
    }
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
            if (dlci_id == 0) {
                _state.is_initiator = 1u; // @todo: TC required for branching
            } else {
                dlci_id_append(dlci_id);                
            }
            os_status = _semaphore.release();
            MBED_ASSERT(os_status == osOK); 
            // @todo: need to verify correct call order for sm change and Semaphore release.
            tx_state_change(TX_IDLE, tx_idle_entry_run);          
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
            tx_state_change(TX_IDLE, tx_idle_entry_run);            
            break;
        default:
            trace("on_rx_frame_dm: ", _tx_context.tx_state);                
            MBED_ASSERT(false);
            break;
    }
}


void Mux::dm_response_send()
{
    dm_response_construct();
                
    const ssize_t return_code = write_do();   
    MBED_ASSERT(return_code != 0);
    if (return_code >= 0) {
        tx_state_change(TX_INTERNAL_RESP, NULL);        
    } else {
        // @todo: propagate write error to user or just discard.
        MBED_ASSERT(false);
    }                   
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


void Mux::valid_rx_frame_decode()
{ 
#if 0    
    trace("valid_rx_frame_decode: ", _rx_context.offset);  
    for (uint8_t i = 0; i != _rx_context.offset; ++i) {
        trace("DECODE BYTE:", _rx_context.buffer[i]);
    }
#endif // 0    
    
    typedef void (*rx_frame_decoder_func_t)();    
    static const rx_frame_decoder_func_t decoder_func[FRAME_RX_TYPE_MAX] = {
        on_rx_frame_sabm,
        on_rx_frame_ua,
        on_rx_frame_dm,
        on_rx_frame_disc,
        on_rx_frame_uih,
        on_rx_frame_not_supported
    };

    const Mux::FrameRxType frame_type = frame_rx_type_resolve();
    rx_frame_decoder_func_t func      = decoder_func[frame_type];
    func();      
    
    _rx_context.offset = 1;    
}


void Mux::decoder_state_decode_run()
{
//trace("decoder_state_decode_run: ", _rx_context.buffer[_rx_context.offset]);
    
    const uint8_t current_byte = _rx_context.buffer[_rx_context.offset];
       
    if (current_byte != CONTROL_ESCAPE_OCTET) {            
        if (current_byte == FLAG_SEQUENCE_OCTET) {
            if (is_rx_frame_valid()) {
                if (is_rx_suspend_requited()) {
                    MBED_ASSERT(false); // // @todo ASSERT for now                    
                } else {
                    valid_rx_frame_decode();
                }                
            } else {
                // @todo: below code needs TC
                if (_rx_context.offset != 1u) {
                    trace("_rx_context.offset: ", _rx_context.offset);    
                    MBED_ASSERT(false); // // @todo invalid frame: ASSERT for now               
                }
            }            
        } else {
            ++_rx_context.offset;
            if (_rx_context.offset == sizeof(_rx_context.buffer)) {           
                MBED_ASSERT(false); // @todo overflow: ASSERT for now            
            }                       
        }                     
    } else {
        MBED_ASSERT(false); // @todo ASSERT for now
    }
}


void Mux::decode_do()
{
    typedef void (*decoder_func_t)();    
    static const decoder_func_t decoder_func[DECODER_STATE_MAX] = {
        decoder_state_sync_run,
        decoder_state_decode_run
    };
    
    decoder_func_t func = decoder_func[_rx_context.decoder_state];
    func();   
}


void Mux::read_do()
{
//trace("read_do::_rx_context.offset: ", _rx_context.offset);    
    
    const ssize_t ret = _serial->read(&(_rx_context.buffer[_rx_context.offset]), READ_LEN);       
    MBED_ASSERT((ret == 1) || (ret < 0));
    decode_do();
}


void Mux::tx_state_change(TxState new_state, tx_state_entry_func_t entry_func)
{
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
            tx_state_change(TX_RETRANSMIT_DONE, tx_retransmit_done_entry_run);
            break;
        default:
            /* Code that should never be reached. */
            trace("_tx_context.tx_state", _tx_context.tx_state);                       
            MBED_ASSERT(false);
            break;
    }       
}


void Mux::on_post_tx_frame_ua()
{
    switch (_tx_context.tx_state) {
        case TX_INTERNAL_RESP:
            if (!_state.is_mux_open) {
                _state.is_mux_open = 1;
                _mux_obj_cb->on_mux_start();                
            } else {
                const uint8_t dlci_id = (_tx_context.buffer[1] >> 2);
                if (!is_dlci_in_use(dlci_id)) {
                    dlci_id_append(dlci_id);
                    _mux_obj_cb->on_dlci_establish(NULL, 0);                    
                }                
            } 
            
            tx_state_change(TX_IDLE, tx_idle_entry_run);            
            break;
        default:
            /* Code that should never be reached. */
            trace("_tx_context.tx_state", _tx_context.tx_state);                       
            MBED_ASSERT(false);
            break;
    }       
}


void Mux::tx_idle_entry_run()
{
    if (_state.is_mux_open_self_iniated_pending) {
        /* Construct the frame, start the tx sequence 1-byte at time, set and reset relevant state contexts. */
        _state.is_mux_open_self_iniated_running = 1u;       
        _state.is_mux_open_self_iniated_pending = 0;
        
        sabm_request_construct(0);
        const ssize_t return_code = write_do();  
        if (return_code >= 0) {        
            tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
            _tx_context.retransmit_counter = RETRANSMIT_COUNT;                
        } else {
            _establish_status        = MUX_ESTABLISH_WRITE_ERROR;
            const osStatus os_status = _semaphore.release();
            MBED_ASSERT(os_status == osOK);
        }
    } else if (_state.is_dlci_open_self_iniated_pending) {
        if (!is_dlci_in_use(_dlci_id)) {
            /* Construct the frame, start the tx sequence 1-byte at time, set and reset relevant state contexts. */      
  
            _state.is_dlci_open_self_iniated_running = 1u;
            _state.is_dlci_open_self_iniated_pending = 0;
            
            sabm_request_construct(_dlci_id);
            const ssize_t return_code = write_do();  
            if (return_code >= 0) {        
                tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
                _tx_context.retransmit_counter = RETRANSMIT_COUNT;                
            } else {
                _establish_status        = MUX_ESTABLISH_WRITE_ERROR;
                const osStatus os_status = _semaphore.release();
                MBED_ASSERT(os_status == osOK);
            }        
        } else {
                _state.is_dlci_open_self_iniated_pending = 0;
                _establish_status                        = MUX_ESTABLISH_MAX;
                const osStatus os_status                 = _semaphore.release();
                MBED_ASSERT(os_status == osOK);
        }
    } else if (_state.is_dlci_open_peer_iniated_pending) {
        const uint8_t dlci_id = _address_field >> 2;           
        if (!is_dlci_in_use(dlci_id)) {       
            /* Construct the frame, start the tx sequence 1-byte at time, set and reset relevant state contexts. */      
    
            _state.is_dlci_open_peer_iniated_running = 1u;
            _state.is_dlci_open_peer_iniated_pending = 0;
                
            ua_response_construct(dlci_id);
            const ssize_t return_code = write_do(); 
            if (return_code >= 0) {       
                tx_state_change(TX_INTERNAL_RESP, NULL);                
            } else {
                // @todo: propagate error to the user.
                MBED_ASSERT(false);
            }
        }
    }
}
 
 
void Mux::on_post_tx_frame_dm()
{
    switch (_tx_context.tx_state) {
        case TX_INTERNAL_RESP:   
            tx_state_change(TX_IDLE, tx_idle_entry_run);           
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
    MBED_ASSERT(false);    
}


Mux::FrameTxType Mux::frame_tx_type_resolve()
{
    const uint8_t frame_type = (_tx_context.buffer[2] & ~PF_BIT);
    
    if (frame_type == FRAME_TYPE_SABM) {
        return FRAME_TX_TYPE_SABM;
    } else if (frame_type == FRAME_TYPE_UA) {
        return FRAME_TX_TYPE_UA;
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


void Mux::on_deferred_call()
{
//trace("Mux::on_deferred_call ", 0);        

    const short events = _serial->poll(POLLIN | POLLOUT);
    if (events & POLLOUT) {
        /* Continue the write sequence if feasible. */
        const ssize_t write_ret = write_do();
//trace("write_ret", write_ret);
//trace("bytes_remaining", _tx_context.bytes_remaining);        
        if (write_ret >= 0) {
            if (_tx_context.bytes_remaining == 0) {
                typedef void (*post_tx_frame_func_t)();   
                static const post_tx_frame_func_t post_tx_func[FRAME_TX_TYPE_MAX] = {
                    on_post_tx_frame_sabm,
                    on_post_tx_frame_ua,
                    on_post_tx_frame_dm,
                    on_post_tx_frame_uih                    
                };
                
                const Mux::FrameTxType frame_type = frame_tx_type_resolve();
                post_tx_frame_func_t func         = post_tx_func[frame_type];
                func();                
                
                // @todo: DEFECT WE MIGTH DO EXTRA TIMER SCHEDULING SHOULD USE DEDICATED FLAG FOR THIS
            }
        } else {
            switch (_tx_context.tx_state) {
                osStatus os_status;
                case TX_RETRANSMIT_ENQUEUE:
                    _establish_status = MUX_ESTABLISH_WRITE_ERROR;
                    os_status         = _semaphore.release();
                    MBED_ASSERT(os_status == osOK);    
                    tx_state_change(TX_IDLE, NULL);                    
                    break;
                default:
                    MBED_ASSERT(false); // @todo write returned < 0 for failure propagate error to the user
                    break;
            }
        }
    } else if (events & POLLIN) {
        read_do();
    } else {
        MBED_ASSERT(false);
    }
        
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


void Mux::callback_attach(MuxCallback *callback)
{
    _mux_obj_cb = callback;
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
        frame_hdr->address = (_state.is_initiator ? 3 : 1) | (dlci_id << 2);                  
    }
    frame_hdr->control        = (FRAME_TYPE_SABM | PF_BIT);            
    frame_hdr->information[0] = fcs_calculate(&(Mux::_tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    _tx_context.bytes_remaining = SABM_FRAME_LEN;
    _tx_context.offset          = 0;                
}


uint8_t Mux::encode_do()
{
    uint8_t       encoded_byte;
    const uint8_t current_byte = _tx_context.buffer[_tx_context.offset];
    
    /* Encoding done only between the opening and closing flag. */
    if ((_tx_context.offset != 0) && (_tx_context.bytes_remaining != 1)) {
        if ((current_byte != FLAG_SEQUENCE_OCTET) && (current_byte != CONTROL_ESCAPE_OCTET)) {
            encoded_byte = current_byte;
        } else {
            MBED_ASSERT(false); // @todo ASSERT for now
            
            _tx_context.buffer[_tx_context.offset] ^= (1 << 5);
            encoded_byte                            = CONTROL_ESCAPE_OCTET;
        }
    } else {
        encoded_byte = current_byte;
    }
    
    return encoded_byte;
}


ssize_t Mux::write_do()
{
    ssize_t write_ret = 0;
    
    if (_tx_context.bytes_remaining != 0) {
        const uint8_t encoded_byte = encode_do();
        
//trace("WRITE: ", _tx_context.offset);
        
        write_ret = _serial->write(&encoded_byte, WRITE_LEN);
        if (write_ret == 1) {
            --(_tx_context.bytes_remaining);
            ++(_tx_context.offset);
        } 
    }
    
    return write_ret;
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
//    FileHandle *obj   = NULL;
    const uint8_t end = sizeof(_mux_objects) / sizeof(_mux_objects[0]);
    for (uint8_t i = 0; i != end; ++i) {   
        if (_mux_objects[i].dlci == MUX_DLCI_INVALID_ID) {
            _mux_objects[i].dlci = dlci_id;
            
            break;
        }
    }
    
//    MBED_ASSERT(i != end);
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
//        Mux::FrameTxType tx_frame_type;        
        int              ret_wait;
        ssize_t          write_err;        
        case TX_IDLE:                
            /* Construct the frame, start the tx sequence 1-byte at time, reset relevant state contexts and suspend 
               the call thread. */                        
            status = MUX_ESTABLISH_SUCCESS;           
            sabm_request_construct(dlci_id);
            write_err = write_do();   
            if (write_err < 0) {
                status = MUX_ESTABLISH_WRITE_ERROR;
// @todo: add mutex_free                                
                return 4u;
            }

            _state.is_dlci_open_self_iniated_running = 1u;            
            tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
            _tx_context.retransmit_counter = RETRANSMIT_COUNT; // SET TO TX_IDLE EXIT
              
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
        Mux::FrameTxType tx_frame_type;
        int              ret_wait;
        ssize_t          write_err;
        case TX_IDLE:
            /* Construct the frame, start the tx sequence 1-byte at time, reset relevant state contexts and suspend 
               the call thread. */           
            sabm_request_construct(0);
            write_err = write_do();   
            if (write_err < 0) {
                status = MUX_ESTABLISH_WRITE_ERROR;
// @todo: add mutex_free                                
                return 2u;
            }
            
            _state.is_mux_open_self_iniated_running = 1u;
            tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
            _tx_context.retransmit_counter = RETRANSMIT_COUNT;
              
// @todo: add mutex_free here               
            ret_wait = _semaphore.wait();
            MBED_ASSERT(ret_wait == 1);
            status = static_cast<MuxEstablishStatus>(_establish_status);
            if (status == MUX_ESTABLISH_SUCCESS) {
                _state.is_mux_open = 1u;                
            }                  
            break;
        case TX_INTERNAL_RESP:
            tx_frame_type = frame_tx_type_resolve();
            if (tx_frame_type == FRAME_TX_TYPE_UA) {
// @todo: add mutex free                
                return 1u;
            } 
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

} // namespace mbed
