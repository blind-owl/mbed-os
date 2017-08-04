
#include "mbed_mux.h"
#include "mbed_assert.h"
#include "EventQueueMock.h"

namespace mbed {

#define FLAG_SEQUENCE_OCTET                 0x7Eu         /* Flag field used in the advanced option mode. */
#define ADDRESS_MUX_START_REQ_OCTET         0x03u         /* Address field of the start multiplexer request frame. */  
/* Address field value of the start multiplexer response frame. */        
#define ADDRESS_MUX_START_RESP_OCTET        ADDRESS_MUX_START_REQ_OCTET                                                 
#define CONTROL_MUX_START_REQ_OCTET         0x3Fu         /* Control field of the start multiplexer request frame. */
/* Control field of the DLCI establishment request frame. */
#define CONTROL_DLCI_START_REQ_OCTET        CONTROL_MUX_START_REQ_OCTET
#define CONTROL_MUX_START_ACCEPT_RESP_OCTET 0x13u         /* Control field of the start multiplexer response frame, 
                                                             peer accept. */
#define CONTROL_MUX_START_REJECT_RESP_OCTET 0x1Fu         /* Control field of the start multiplexer response frame, 
                                                             peer reject. */
#define FCS_INPUT_LEN                       2u            /* Length of the input for FCS calculation in number of 
                                                             bytes. */
#define CONTROL_ESCAPE_OCTET                0x7Du         /* Control escape octet used as the transparency 
                                                             identifier. */
#define WRITE_LEN                           1u            /* Length of write in number of bytes. */    
#define READ_LEN                            1u            /* Length of read in number of bytes. */    
#define START_REQ_FRAME_LEN                 5u            /* Length of ther start request frame in number of bytes. */
#define DLCI_ESTABLISH_REQ_FRAME_LEN        5u            /* Length of ther DLCI establishment request frame in number 
                                                             of bytes. */
#define T1_TIMER_VALUE                      300u          /* T1 timer value. */
#define RETRANSMIT_COUNT                    3u            /* Retransmission count for the tx frames requiring a
                                                             response. */
    
/* Definition for frame header type. */
typedef struct
{
    uint8_t flag_seq;       /* Flag sequence field. */
    uint8_t address;        /* Address field. */
    uint8_t control;        /* Control field. */
    uint8_t information[1]; /* Begin of the information field if present. */
} frame_hdr_t;    

FileHandle *Mux::_serial      = NULL;
EventQueueMock *Mux::_event_q = NULL;
MuxCallback *Mux::_mux_obj_cb = NULL;

//rtos::Semaphore Mux::_semaphore(0);
SemaphoreMock Mux::_semaphore;

Mux::tx_context_t Mux::tx_context;
Mux::rx_context_t Mux::rx_context;
Mux::state_t      Mux::state;
const uint8_t     Mux::crctable[MUX_CRC_TABLE_LEN] = {
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
    state.is_multiplexer_open = 0;
    state.is_request_timeout  = 0;
    state.is_initiator        = 0;
   
    rx_context.offset        = 0;
    rx_context.decoder_state = DECODER_STATE_SYNC;    
    
    tx_context.tx_state = TX_IDLE;    
}


void Mux::frame_retransmit_begin()
{   
//trace("Mux::frame_retransmit ", 0);    
    tx_context.bytes_remaining = tx_context.offset;
    tx_context.offset          = 0;
   
    const ssize_t ret_write = write_do();   
    if (ret_write < 0) {
        MBED_ASSERT(false); // @todo propagate error to user.
    }       
}


void Mux::on_timeout()
{
//trace("on_timeout:tx_context.tx_state", tx_context.tx_state);    
// ADD STATE STUFF
//trace("Mux::on_timeout ", tx_context.retransmit_counter);     
    switch (tx_context.tx_state) {
        case TX_RETRANSMIT_DONE:
            if (tx_context.retransmit_counter != 0) {
                --(tx_context.retransmit_counter);
                frame_retransmit_begin();
                tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
            } else {
                /* Retransmission limit reachd, change state and release the suspended call thread with appropriate 
                   status code. */
                state.is_request_timeout = 1;
                const osStatus os_status = _semaphore.release();
                MBED_ASSERT(os_status == osOK);    
                tx_state_change(TX_IDLE, NULL);
                // @todo: need to be carefull of call order between the thread release and state change.
            }            
            break;
        default:
            /* No implementtaion required. */
            break;
    }
}


void Mux::decoder_state_change(DecoderState new_state)
{
    rx_context.decoder_state = new_state;
}


void Mux::decoder_state_sync_run()
{
//trace("decoder_state_sync_run: ", rx_context.buffer[rx_context.offset]);    

    if (rx_context.buffer[rx_context.offset] == FLAG_SEQUENCE_OCTET) {
        ++rx_context.offset;
        decoder_state_change(DECODER_STATE_DECODE);
    }
}


bool Mux::is_rx_frame_valid()
{
//    trace("is_rx_frame_valid::rx_context.offset: ", rx_context.offset);    
    
    return (rx_context.offset >= 4) ? true : false;
}


bool Mux::is_rx_suspend_requited()
{
    // @todo: implement me!    
    return false;
}


void Mux::start_response_construct()
{
    // @todo: combine common functionality with request function.
    
    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::tx_context.buffer[0]));
    
    frame_hdr->flag_seq       = FLAG_SEQUENCE_OCTET;
    frame_hdr->address        = ADDRESS_MUX_START_RESP_OCTET;
    frame_hdr->control        = CONTROL_MUX_START_ACCEPT_RESP_OCTET;    
    frame_hdr->information[0] = fcs_calculate(&(Mux::tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    // @todo: make START_RESP_FRAME_LEN define
    
    tx_context.bytes_remaining = START_REQ_FRAME_LEN;
    tx_context.offset          = 0;       
}


void Mux::dlci_0_establish_req_do()
{
//trace("dlci_0_establish_req_do ", 0);    

    // @todo: assume mux START request internal state inprogress/pending only for now!
    
    switch (tx_context.tx_state) {
        ssize_t return_code;
        case TX_IDLE:
            /* Construct the frame, start the tx sequence 1-byte at time, reset relevant state contexts. */             
            start_response_construct();  
            return_code = write_do();    
            MBED_ASSERT(return_code != 0);
            if (return_code > 0) {
                tx_state_change(TX_INTERNAL_RESP, NULL);
            } else {
                // @todo: propagate write error to user.
            }
            break;
        case TX_RETRANSMIT_ENQUEUE:
        case TX_RETRANSMIT_DONE:            
            // @todo: assume mux START request internal state inprogress/pending only for now!
            break;
        default:
            trace("dlci_0_establish_req_do: ", tx_context.tx_state);    
            MBED_ASSERT(false);
            break;        
    }
}


void Mux::dlci_0_establish_resp_do()
{
//trace("dlci_0_establish_resp_do ", 0);    

    // @todo: verify that we have issued the start request

    switch (tx_context.tx_state) {
        osStatus os_status;
        case TX_RETRANSMIT_DONE:
            _event_q->cancel(tx_context.timer_id);
            
            // @todo: need to verify correct call order for sm change and Semaphore release.
            tx_state_change(TX_IDLE, NULL);
            os_status = _semaphore.release();
            MBED_ASSERT(os_status == osOK); 
                            
            // @todo: DEFECT ONLY OPEN AFTER RX RESPONSE SUCCESS DECODED
            // TC: 
            // - peer reject mux open
            // - reissue mux start without init in between
            state.is_multiplexer_open = 1; 
            break;
        default:
            trace("dlci_0_establish_resp_do: ", tx_context.tx_state);                
            MBED_ASSERT(false);
            break;
    }
}


void Mux::rx_frame_not_supported_decode_do()
{
//    trace("rx_frame_not_supported_decode_do ", 0);    
    MBED_ASSERT(false);    
}
        
        
Mux::RxFrameType Mux::valid_rx_frame_decode_do()
{
//trace("valid_rx_frame_decode_do ", rx_context.buffer[2]);    

    // @todo: address field decoding to be added for compare
    
    if ((rx_context.buffer[2] == CONTROL_MUX_START_ACCEPT_RESP_OCTET) || 
        (rx_context.buffer[2] == CONTROL_MUX_START_REJECT_RESP_OCTET)) {
        return RX_FRAME_DLCI_0_ESTABLISH_RESPONSE;
    } else if (rx_context.buffer[2] == CONTROL_MUX_START_REQ_OCTET) {
        return RX_FRAME_DLCI_0_ESTABLISH_REQUEST;
    } else {
        return RX_FRAME_NOT_SUPPORTED;
    }

}


void Mux::valid_rx_frame_decode()
{       
    typedef void (*rx_frame_decoder_func_t)();    
    static const rx_frame_decoder_func_t decoder_func[RX_FRAME_TYPE_MAX] = {
        dlci_0_establish_req_do,
        dlci_0_establish_resp_do,
        rx_frame_not_supported_decode_do
    };

    const Mux::RxFrameType frame_type = valid_rx_frame_decode_do();
    rx_frame_decoder_func_t func      = decoder_func[frame_type];
    func();      
    
    rx_context.offset = 1;    
}


void Mux::decoder_state_decode_run()
{
//trace("decoder_state_decode_run: ", rx_context.buffer[rx_context.offset]);
    
    const uint8_t current_byte = rx_context.buffer[rx_context.offset];
       
    if (current_byte != CONTROL_ESCAPE_OCTET) {            
        if (current_byte == FLAG_SEQUENCE_OCTET) {
            if (is_rx_frame_valid()) {
                if (is_rx_suspend_requited()) {
                    MBED_ASSERT(false); // // @todo ASSERT for now                    
                } else {
                    valid_rx_frame_decode();
                }                
            } else {
                MBED_ASSERT(false); // // @todo invalid frame: ASSERT for now               
            }            
        } else {
            ++rx_context.offset;
            if (rx_context.offset == sizeof(rx_context.buffer)) {           
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
    
    decoder_func_t func = decoder_func[rx_context.decoder_state];
    func();   
}


void Mux::read_do()
{
//trace("read_do::rx_context.offset: ", rx_context.offset);    
    
    const ssize_t ret = _serial->read(&(rx_context.buffer[rx_context.offset]), READ_LEN);       
    MBED_ASSERT((ret == 1) || (ret < 0));
    decode_do();
}


void Mux::tx_state_change(TxState new_state, tx_state_entry_func_t entry_func)
{
    tx_context.tx_state = new_state;
    
//    trace("tx_state_change: ", tx_context.tx_state);
    
    if (entry_func != NULL) {
        entry_func();
    }
}


void Mux::tx_retransmit_done_entry_run()
{
    tx_context.timer_id = _event_q->call_in(T1_TIMER_VALUE, Mux::on_timeout);
    MBED_ASSERT(tx_context.timer_id != 0);                
}
 
 
void Mux::on_deferred_call()
{
//trace("Mux::on_deferred_call ", 0);        

    const short events = _serial->poll(POLLIN | POLLOUT);
    if (events & POLLOUT) {
        /* Continue the write sequence if feasible. */
        const ssize_t write_ret = write_do();
//trace("write_ret", write_ret);
//trace("bytes_remaining", tx_context.bytes_remaining);        
        if (write_ret >= 0) {
            if (tx_context.bytes_remaining == 0) { 
                
                // @todo: DEFECT WE MIGTH DO EXTRA TIMER SCHEDULING SHOULD USE DEDICATED FLAG FOR THIS         
#if 1
//trace("on_deferred_call:tx_context.tx_state", tx_context.tx_state);
                switch (tx_context.tx_state) {
                    case TX_RETRANSMIT_ENQUEUE:
                        tx_state_change(TX_RETRANSMIT_DONE, tx_retransmit_done_entry_run);
                        break;
                    case TX_INTERNAL_RESP:
                        if (!state.is_multiplexer_open) {
                            state.is_multiplexer_open = 1;
                            _mux_obj_cb->on_mux_start();
                        }
                        
                        tx_state_change(TX_IDLE, NULL);
                        break;
                    default:
                        /* No implementtaion required. */
                        MBED_ASSERT(false);
                        break;
                }
#endif //                 
#if 0                
                tx_context.timer_id = _event_q->call_in(T1_TIMER_VALUE, Mux::on_timeout);
                MBED_ASSERT(tx_context.timer_id != 0);            
#endif // 0 replaced by above switc...case                
            }
        } else {
            MBED_ASSERT(false); // @todo write returned < 0 for failure propagate error to the user
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
        fcs = crctable[fcs^*buffer++];
    }
    
    /* Ones complement. */
    fcs = 0xFFu - fcs;
 
//    trace("FCS: ", fcs);

    return fcs;
}


void Mux::start_request_construct()
{
    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::tx_context.buffer[0]));
    
    frame_hdr->flag_seq       = FLAG_SEQUENCE_OCTET;
    frame_hdr->address        = ADDRESS_MUX_START_REQ_OCTET;
    frame_hdr->control        = CONTROL_MUX_START_REQ_OCTET;    
    frame_hdr->information[0] = fcs_calculate(&(Mux::tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    tx_context.bytes_remaining = START_REQ_FRAME_LEN;
    tx_context.offset          = 0;        
}


void Mux::dlci_establish_request_construct(uint8_t dlci_id)
{
    // @todo: combine with start request
    
    frame_hdr_t *frame_hdr = reinterpret_cast<frame_hdr_t *>(&(Mux::tx_context.buffer[0]));
    
    frame_hdr->flag_seq       = FLAG_SEQUENCE_OCTET;
    
    const uint8_t address = 3 | (dlci_id >> 2); // @todo: hard code as initiator for now
    
    frame_hdr->address        = address;
    frame_hdr->control        = CONTROL_DLCI_START_REQ_OCTET;    
    frame_hdr->information[0] = fcs_calculate(&(Mux::tx_context.buffer[1]), FCS_INPUT_LEN);    
    (++frame_hdr)->flag_seq   = FLAG_SEQUENCE_OCTET;
    
    tx_context.bytes_remaining = DLCI_ESTABLISH_REQ_FRAME_LEN;
    tx_context.offset          = 0;            
}


uint8_t Mux::encode_do()
{
    uint8_t       encoded_byte;
    const uint8_t current_byte = tx_context.buffer[tx_context.offset];
    
    /* Encoding done only between the opening and closing flag. */
    if ((tx_context.offset != 0) && (tx_context.bytes_remaining != 1)) {
        if ((current_byte != FLAG_SEQUENCE_OCTET) && (current_byte != CONTROL_ESCAPE_OCTET)) {
            encoded_byte = current_byte;
        } else {
            MBED_ASSERT(false); // @todo ASSERT for now
            
            tx_context.buffer[tx_context.offset] ^= (1 << 5);
            encoded_byte                          = CONTROL_ESCAPE_OCTET;
        }
    } else {
        encoded_byte = current_byte;
    }
    
    return encoded_byte;
}


ssize_t Mux::write_do()
{
    ssize_t write_ret = 0;
    
    if (tx_context.bytes_remaining != 0) {
        const uint8_t encoded_byte = encode_do();
        write_ret                  = _serial->write(&encoded_byte, WRITE_LEN);   
        MBED_ASSERT((write_ret == 1) || (write_ret < 0)); // @todo: FIX ME: can also return 0 if called from on_timeout
        if (write_ret == 1) {
            --(tx_context.bytes_remaining);
            ++(tx_context.offset);
        } 
    }
    
    return write_ret;
}


Mux::MuxEstablishStatus Mux::start_response_decode()
{
    Mux::MuxEstablishStatus status;
    const frame_hdr_t      *frame = reinterpret_cast<const frame_hdr_t*>(&(rx_context.buffer[0]));
    
    switch (frame->control) {
        case CONTROL_MUX_START_ACCEPT_RESP_OCTET:
            status = MUX_ESTABLISH_SUCCESS;
            break;
        case CONTROL_MUX_START_REJECT_RESP_OCTET:
            status = MUX_ESTABLISH_REJECT;
            break;
        default:
            MBED_ASSERT(false);
            break;
    }
    
    return status;
}


ssize_t Mux::mux_start(Mux::MuxEstablishStatus &status)
{
    ssize_t return_code;
    
    if (state.is_multiplexer_open) {
        return 0;
    }
   
    switch (tx_context.tx_state) {
        case TX_IDLE:
            /* Construct the frame, start the tx sequence 1-byte at time, reset relevant state contexts and suspend 
               the call thread. */            
            start_request_construct();                       
            return_code = write_do();    
            MBED_ASSERT(return_code != 0);
            if (return_code > 0) {
                return_code = 2;
                tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
                state.is_request_timeout      = 0;    
                tx_context.retransmit_counter = RETRANSMIT_COUNT;                
                const int ret_wait = _semaphore.wait();
                MBED_ASSERT(ret_wait == 1);                
                /* Decode response frame from the rx buffer in order to set the correct status code if no request 
                   timeout occurred. */ 
                if (!state.is_request_timeout) {
                    status = start_response_decode();
                } else {
                    status = MUX_ESTABLISH_TIMEOUT;
                }            
            }                                     
            break;
        default:
            MBED_ASSERT(false);
            break;
    };
    
    return return_code;   
}


ssize_t Mux::dlci_establish(uint8_t dlci_id, MuxEstablishStatus &status)
{
    ssize_t return_code;
    
    if (!state.is_multiplexer_open) {
        return 1;
    }
    
    switch (tx_context.tx_state) {
        case TX_IDLE:                
            status = MUX_ESTABLISH_SUCCESS;
            
            dlci_establish_request_construct(dlci_id);  
//trace("dlci_establish ", 0);                    
            return_code = write_do();    
            MBED_ASSERT(return_code != 0);    
            return_code = 2;
            tx_state_change(TX_RETRANSMIT_ENQUEUE, NULL);
            const int ret_wait = _semaphore.wait();
            MBED_ASSERT(ret_wait == 1);
            break;
        default:
            MBED_ASSERT(false);
            break;
    }

    return return_code;   
}

} // namespace mbed
