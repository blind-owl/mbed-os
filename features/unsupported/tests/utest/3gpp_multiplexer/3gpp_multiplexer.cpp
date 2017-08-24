#include "TestHarness.h"
#include "mock.h"
#include "mbed_mux.h"
#include "FileHandleMock.h"
#include "EventQueueMock.h"

namespace mbed {

void trace(char *string, int data)
{
    UT_PRINT_LOCATION(string, "TRACE: ", data);
}


class MuxClient : public mbed::MuxCallback {
public:
    
    virtual void on_mux_start();       
    virtual void on_dlci_establish(FileHandle *obj, uint8_t dlci_id);
    virtual void event_receive() {};    
    
    static void reset(); 
    static bool is_mux_start_triggered();
    static bool is_dlci_establish_triggered();
   
    MuxClient() {};
private:
    
    static bool _is_mux_start_triggered;
    static bool _is_dlci_establish_triggered;
};

bool MuxClient::_is_mux_start_triggered      = false;
bool MuxClient::_is_dlci_establish_triggered = false;

void MuxClient::reset()
{
    _is_mux_start_triggered      = false;
    _is_dlci_establish_triggered = false;
}
 
 
bool MuxClient::is_mux_start_triggered()
{
    const bool ret          = _is_mux_start_triggered;
    _is_mux_start_triggered = false;
    
    return ret;
}


void MuxClient::on_mux_start()
{
    _is_mux_start_triggered = true;
}


bool MuxClient::is_dlci_establish_triggered()
{
    const bool ret               = _is_dlci_establish_triggered;
    _is_dlci_establish_triggered = false;
    
    return ret;
}
    
    
void MuxClient::on_dlci_establish(FileHandle *obj, uint8_t dlci_id)
{
    _is_dlci_establish_triggered = true;
}

static MuxClient mux_client;

TEST_GROUP(MultiplexerOpenTestGroup)
{
    void setup()
    {
        mock_init();
        Mux::module_init();
        mux_client.reset();
        mbed::Mux::callback_attach(&mux_client);
    }

    void teardown()
    {
        mock_verify();
    }
};


TEST(MultiplexerOpenTestGroup, FirstTest)
{
    
    /* These checks are here to make sure assertions outside test runs don't crash */
    CHECK(true);
    LONGS_EQUAL(1, 1);
    STRCMP_EQUAL("mbed SDK!", "mbed SDK!");
}

#define MUX_START_FRAME_LEN          5u                          /* Length of the multiplexer start frame in number of 
                                                                    bytes. */
#define WRITE_LEN                    1u                          /* Length of single write call in number of bytes. */  
#define READ_LEN                     1u                          /* Length of single read call in number of bytes. */
#define FLAG_SEQUENCE_OCTET          0x7Eu                       /* Flag field used in the advanced option mode. */
#define ADDRESS_MUX_START_REQ_OCTET  0x03u                       /* Address field value of the start multiplexer 
                                                                    request frame. */
#define ADDRESS_MUX_START_RESP_OCTET ADDRESS_MUX_START_REQ_OCTET /* Address field value of the start multiplexer 
                                                                    response frame. */
#define T1_TIMER_VALUE               300u                        /* T1 timer value. */
#define T1_TIMER_EVENT_ID            1                           /* T1 timer event id. */
#define CRC_TABLE_LEN                256u                        /* CRC table length in number of bytes. */
#define RETRANSMIT_COUNT             3u                          /* Retransmission count for the tx frames requiring a 
                                                                    response. */       
#define FRAME_TYPE_SABM              0x2Fu                       /* SABM frame type coding in the frame control 
                                                                    field. */
#define FRAME_TYPE_UA                0x63u                       /* UA frame type coding in the frame control field. */
#define FRAME_TYPE_DM                0x0Fu                       /* DM frame type coding in the frame control field. */
#define FRAME_TYPE_DISC              0x43u                       /* DISC frame type coding in the frame control 
                                                                    field. */
#define FRAME_TYPE_UIH               0xEFu                       /* UIH frame type coding in the frame control field. */
#define PF_BIT                       (1u << 4)                   /* P/F bit position in the frame control field. */     
#define DLCI_ID_LOWER_BOUND          1u                          /* Lower bound DLCI id value. */ 
#define DLCI_ID_UPPER_BOUND          63u                         /* Upper bound DLCI id value. */ 
                      
static const uint8_t crctable[CRC_TABLE_LEN] = {
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

uint8_t fcs_calculate(const uint8_t *buffer,  uint8_t input_len)
{
    uint8_t fcs = 0xFFu;

    while (input_len-- != 0) {
        fcs = crctable[fcs^*buffer++];
    }
    
    /* Ones complement. */
    fcs = 0xFFu - fcs;

    return fcs;
}


/*
 * LOOP UNTIL COMPLETE REQUEST FRAME WRITE DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call poll
 * - call write
 * - call call_in in the last iteration for T1 timer
 * - CALL RETURN 
 */
void self_iniated_request_tx(const uint8_t *tx_buf, uint8_t tx_buf_len)
{
    /* Write the complete request frame in the do...while. */
    uint8_t                                  tx_count      = 0;           
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};    
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};
    do {    
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback with POLLOUT event from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;
        
        mbed::FileHandleMock::io_control(io_control);

        /* Trigger deferred call from EventQueue.
         * Continue with the frame write sequence. */
        mock_t * mock_poll      = mock_free_get("poll");    
        CHECK(mock_poll != NULL);         
        mock_poll->return_value = POLLOUT;
        mock_t * mock_write     = mock_free_get("write");
        CHECK(mock_write != NULL); 
        
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(tx_buf[tx_count]);        
        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        

        if (tx_count == tx_buf_len - 1) {
            
            /* Start frame write sequence gets completed, now start T1 timer. */   
            
            mock_t * mock_call_in = mock_free_get("call_in");    
            CHECK(mock_call_in != NULL);     
            mock_call_in->return_value = T1_TIMER_EVENT_ID;        
            mock_call_in->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_call_in->input_param[0].param        = T1_TIMER_VALUE;                  
        }
        
        mbed::EventQueueMock::io_control(eq_io_control);   
        
        ++tx_count;        
    } while (tx_count != tx_buf_len);
}


/*
 * LOOP UNTIL COMPLETE RESPONSE FRAME READ DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call poll
 * - call read
 * - call cancel in the last iteration to cancel T1 timer
 * - CALL RETURN 
 */
void self_iniated_response_rx(const uint8_t *rx_buf, uint8_t rx_buf_len)
{
    /* Read the complete response frame in do...while. */
    uint8_t                                  rx_count      = 0;       
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    do {
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback with POLLIN event from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;

        mbed::FileHandleMock::io_control(io_control);

        /* Trigger deferred call from EventQueue.
         * Continue with the frame read sequence. */
        mock_t * mock_poll = mock_free_get("poll");    
        CHECK(mock_poll != NULL);         
        mock_poll->return_value = POLLIN;
        mock_t * mock_read      = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 1;  

        if (rx_count == (rx_buf_len - 1)) {
            
            /* Start frame read sequence gets completed, now cancel T1 timer. */   
            
            mock_t * mock_cancel = mock_free_get("cancel");    
            CHECK(mock_cancel != NULL);    
            mock_cancel->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_cancel->input_param[0].param        = T1_TIMER_EVENT_ID;     
            
            /* Release the semaphore blocking the call thread. */
            mock_t * mock_release = mock_free_get("release");
            CHECK(mock_release != NULL);
            mock_release->return_value = osOK;
        }

        mbed::EventQueueMock::io_control(eq_io_control);   

        ++rx_count;        
    } while (rx_count != rx_buf_len);           
}


/*
 * LOOP UNTIL COMPLETE REQUEST FRAME READ DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call poll
 * - call read
 * - begin response frame TX sequence in the last iteration if parameter supplied
 * - CALL RETURN 
 */
void peer_iniated_request_rx(const uint8_t *rx_buf, uint8_t rx_buf_len, const uint8_t *write_byte)
{
    /* Read the complete request frame in do...while. */
    uint8_t                                  rx_count      = 0;      
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    do {
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback with POLLIN event from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;

        mbed::FileHandleMock::io_control(io_control);

        /* Trigger deferred call from EventQueue.
         * Continue with the frame read sequence. */
        mock_t * mock_poll = mock_free_get("poll");    
        CHECK(mock_poll != NULL);         
        mock_poll->return_value = POLLIN;
        mock_t * mock_read      = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 1; 
        
        if (write_byte != NULL)  {
            if (rx_count == (rx_buf_len - 1)) {                
                /* RX frame gets completed start the response frame TX sequence. */                  
                mock_t * mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)write_byte;                      
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 1;
            }
        }
        
        mbed::EventQueueMock::io_control(eq_io_control);   

        ++rx_count;        
    } while (rx_count != rx_buf_len);    
}


/*
 * LOOP UNTIL COMPLETE RESPONSE FRAME WRITE DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call poll
 * - call write
 * - verify completion callback state in the last iteration, if supplied
 * - write 1st byte of new pending frame in the last iteration, if supplied
 * - CALL RETURN 
 */
typedef bool (*compare_func_t)();
void peer_iniated_response_tx(const uint8_t *buf,
                              uint8_t        buf_len, 
                              const uint8_t *new_tx_byte,
                              bool           expected_state,
                              compare_func_t func)
{
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    uint8_t                                  tx_count      = 0;          
   
    /* Write the complete response frame in do...while. */
    do {   
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback with POLLOUT event from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;
        
        mbed::FileHandleMock::io_control(io_control);

        /* Trigger deferred call from EventQueue.
         * Continue with the frame write sequence. */
        mock_t * mock_poll      = mock_free_get("poll");    
        CHECK(mock_poll != NULL);         
        mock_poll->return_value = POLLOUT;
        mock_t * mock_write     = mock_free_get("write");
        CHECK(mock_write != NULL); 
        
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(buf[tx_count]);        
        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        

        if (tx_count == (buf_len - 1)) {       
            if (new_tx_byte != NULL) {
                /* Last byte of the response frame written, write 1st byte of new pending frame. */    
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)new_tx_byte;                       
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 1;                           
            }
        }

        mbed::EventQueueMock::io_control(eq_io_control);  

        if (tx_count == (buf_len - 1)) {
            if (func != NULL) {
                /* Last byte of the response frame written, verify correct completion callback state. */    
                CHECK_EQUAL(func(), expected_state);
            }
        }
        
        ++tx_count;        
    } while (tx_count != buf_len);
}


/* Multiplexer semaphore wait call from self initiated multiplexer open TC(s). */
void mux_start_self_initated_sem_wait(const void *context)
{
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };
    const uint8_t write_byte[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte));
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte));    
}


/* Do successfull multiplexer self iniated open.*/
void mux_self_iniated_open()
{      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_start_self_initated_sem_wait;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);    
}


/*
 * TC - mux start-up sequence, self initiated: successfull establishment
 * - issue START request
 * - receive START response
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_succes)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    CHECK(!MuxClient::is_mux_start_triggered());                    
}


/* Multiplexer semaphore wait call from mux_open_self_initiated_existing_open_pending TC. */
void mux_start_self_initated_existing_open_pending_sem_wait(const void *context)
{
    /* Issue new self iniated mux open, which fails. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(ret, 1);
    
    /* Finish the mux open establishment with success. */
    mux_start_self_initated_sem_wait(NULL);
}


/*
 * TC - mux start-up sequence, self initiated: issue new mux open while existing self iniated mux open is in TX cycle
 * - start self iniated mux open: start the TX cycle but don't finish it
 * - issue new self iniated mux open -> returns appropriate error code
 * - finish the mux open establishment cycle with success
 * - issue new self iniated mux open -> returns appropriate error code
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_existing_open_pending)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_start_self_initated_existing_open_pending_sem_wait;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    int ret = mbed::Mux::mux_start(status);    
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);        
    CHECK(!MuxClient::is_mux_start_triggered());    
    
    /* Issue new self iniated mux open, which fails. */
    ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(0, ret);    
}


/* Multiplexer semaphore wait call from mux_open_self_initiated_existing_open_pending_2 TC. */
void mux_start_self_initated_existing_open_pending_2_sem_wait(const void *context)
{
    /* Issue new self iniated mux open, which fails. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(ret, 1);
       
    const uint8_t *dlci_id      = static_cast<const uint8_t*>(context);    
    const uint8_t write_byte[4] = 
    {
        3u | (*dlci_id << 2),
        (FRAME_TYPE_DM | PF_BIT),        
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };       
    const uint8_t new_write_byte = FLAG_SEQUENCE_OCTET;
    
    /* Finish the mux open establishment with success. */    
    peer_iniated_response_tx(&write_byte[0], sizeof(write_byte), &new_write_byte, false, NULL);
    
    /* DM response completed, complete the mux start-up establishent. */
    const uint8_t read_byte[4] = 
    {
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };    
    const uint8_t write_byte_2[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte_2[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_byte_2[0]), sizeof(write_byte_2));
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte));    
}


/*
 * TC - mux start-up sequence, self initiated: issue new mux open while existing self iniated mux open is pending for
 * TX cycle
 * - peer sends a DISC command to DLCI 0 
 * - send 1st byte of DM response
 * - start self iniated mux open: TX cycle NOT started but put pending, as TX DM allready inprogress
 * - issue new self iniated mux open -> returns appropriate error code
 * - finish the mux open establishment cycle with success
 * - issue new self iniated mux open -> returns appropriate error code
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_existing_open_pending_2)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
  
    const uint8_t dlci_id      = 0;
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        /* Peer assumes the role of initiator. */
        3u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };           

    /* Generate DISC from peer and trigger TX of 1st response byte of DM. */        
    const uint8_t write_byte = FLAG_SEQUENCE_OCTET;
    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte), &write_byte);    
    
    /* Issue multiplexer start while DM is in progress. */
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_existing_open_pending_2_sem_wait;    
    mock_wait->func_context = dlci_id;
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    int ret = mbed::Mux::mux_start(status);    
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);        
    CHECK(!MuxClient::is_mux_start_triggered());    
    
    /* Issue new self iniated mux open, which fails. */
    ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(0, ret);    
}


/* Definition for the multiplexer role type. */
typedef enum
{
    ROLE_INITIATOR = 0, 
    ROLE_RESPONDER,
    ROLE_MAX,
} Role;


/* Definition for the dlci establish context. */
typedef struct 
{
    uint8_t dlci_id;
    Role    role;
} dlci_establish_context_t;
    

/* Multiplexer semaphore wait call from self initiated dlci establish TC(s). 
 * Role: initiator
 */
void dlci_establish_self_initated_sem_wait(const void *context)
{
    const dlci_establish_context_t *cntx = static_cast<const dlci_establish_context_t *>(context);
    
    const uint8_t read_byte[4]  = 
    {
        (((cntx->role == ROLE_INITIATOR) ? 1 : 3) | (cntx->dlci_id << 2)),
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };    
    const uint8_t address       = ((cntx->role == ROLE_INITIATOR) ? 3 : 1) | (cntx->dlci_id << 2);    
    const uint8_t write_byte[4] = 
    {
        address, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };    

    /* Complete the request frame write and read the response frame. */
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte));
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte));
}


/* Do successfull self iniated dlci establishment.*/
void dlci_self_iniated_establish(Role role, uint8_t dlci_id)
{
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait;
    
    const dlci_establish_context_t context = {dlci_id, role};
    mock_wait->func_context                = &context;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj = NULL;
    const int ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, 3);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);      
    CHECK(obj != NULL);
    CHECK(!MuxClient::is_dlci_establish_triggered());
}


/*
 * TC - dlci establishment sequence, self initiated: mux is not open
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_iniated_mux_not_open)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);

    /* Start test sequence. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    FileHandle *obj = NULL;    
    const int ret = mbed::Mux::dlci_establish(1, status, &obj);
    CHECK_EQUAL(ret, 1);
    CHECK(!MuxClient::is_dlci_establish_triggered());                        
    CHECK_EQUAL(obj, NULL);    
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: successfull establishment
 * - self iniated open multiplexer
 * - issue DLCI establishment request
 * - receive DLCI establishment response
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_role_initiator_succes)
{   
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    dlci_self_iniated_establish(ROLE_INITIATOR, 1);
}


void dlci_establish_self_initated_sem_wait_timeout(const void * context)
{
    const dlci_establish_context_t *cntx = static_cast<const dlci_establish_context_t *>(context);

    const uint8_t address       = ((cntx->role == ROLE_INITIATOR) ? 3 : 1) | (cntx->dlci_id << 2);        
    const uint8_t write_buf[4]  = 
    {
        address, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_buf[0], 2),
        FLAG_SEQUENCE_OCTET
    };    

    /* Complete the request frame write. */
    self_iniated_request_tx(&(write_buf[0]), sizeof(write_buf));
    
    /* --- begin frame re-transmit sequence --- */

    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_TIMEOUT_GENERATE};
    uint8_t                                  counter       = RETRANSMIT_COUNT;
    do {    
        mock_t * mock_write = mock_free_get("write");
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
        mock_write->input_param[0].param        = (uint32_t)&write_byte;        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    

        /* Trigger timer timeout. */
        mbed::EventQueueMock::io_control(eq_io_control);
        
        /* Re-transmit the complete remaining part of the frame. */
        self_iniated_request_tx(&(write_buf[0]), sizeof(write_buf));
        
        --counter;
    } while (counter != 0);
    
    /* Trigger timer to finish the re-transmission cycle and the whole request. Release the semaphore blocking the call 
       thread. */
    mock_t * mock_release = mock_free_get("release");
    CHECK(mock_release != NULL);
    mock_release->return_value = osOK;    
    mbed::EventQueueMock::io_control(eq_io_control);
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: request timeout failure:
 * - self iniated open multiplexer
 * - issue DLCI establishment request
 * - request timeout timer expires 
 * - generate timeout event to the user
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_timeout)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait_timeout;
    
    const dlci_establish_context_t context = {1, ROLE_INITIATOR};
    mock_wait->func_context                = &context;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj = NULL;        
    const int ret = mbed::Mux::dlci_establish(context.dlci_id, status, &obj);
    CHECK_EQUAL(ret, 3);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);           
    CHECK_EQUAL(obj, NULL);    
    CHECK(!MuxClient::is_dlci_establish_triggered());
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: request timeout failure:
 * - self iniated open multiplexer
 * - issue DLCI establishment request
 * - request timeout timer expires 
 * - generate timeout event to the user
 * - send 2nd establishment request
 * - success
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_success_after_timeout)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait_timeout;
    
    const dlci_establish_context_t context = {1, ROLE_INITIATOR};
    mock_wait->func_context                = &context;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj = NULL;        
    const int ret = mbed::Mux::dlci_establish(context.dlci_id, status, &obj);
    CHECK_EQUAL(ret, 3);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);           
    CHECK_EQUAL(obj, NULL);    
    CHECK(!MuxClient::is_dlci_establish_triggered());    
    
    /* 2nd try - success. */
    dlci_self_iniated_establish(ROLE_INITIATOR, 1);
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: DLCI id allready used
 * - self iniated open multiplexer
 * - issue DLCI establishment request: success
 * - issue DLCI establishment request, same id used: failure
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_dlci_id_used)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1;
    dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);
   
    /* 2nd establishment with the same DLCI id - fail. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    FileHandle *obj = NULL;
    const int ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, 0);    
    CHECK_EQUAL(obj, NULL);
}

#define MAX_DLCI_COUNT 3u

/*
 * TC - dlci establishment sequence, self initiated, role initiator: all DLCI ids used
 * - self iniated open multiplexer
 * - establish DLCIs max amount: success
 * - issue DLCI establishment request: failure as max count reached
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_all_dlci_ids_used)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    uint8_t i       = MAX_DLCI_COUNT;
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    do {
        dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);
       
        --i;
        ++dlci_id;
    } while (i != 0);
    
    /* All available DLCI ids consumed. Next request will fail. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    FileHandle *obj = NULL;
    const int ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, 0);    
    CHECK_EQUAL(obj, NULL);    
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: write failure
 * - write request returns error code, which is forwarded to the user
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_write_failure)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();    
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;                
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = (uint32_t)-1;        
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj = NULL;
    const int ret = mbed::Mux::dlci_establish(1, status, &obj);
    CHECK_EQUAL(ret, -1);
    CHECK(!MuxClient::is_dlci_establish_triggered());    
    CHECK_EQUAL(obj, NULL);
}


/* Multiplexer semaphore wait call from dlci_establish_self_initiated_rejected_by_peer TC. 
 * Role: initiator
 */
void dlci_establish_self_initated_sem_wait_rejected_by_peer(const void *context)
{
    const dlci_establish_context_t *cntx = static_cast<const dlci_establish_context_t *>(context);
    
    const uint8_t read_byte[4] = 
    {
        (((cntx->role == ROLE_INITIATOR) ? 1 : 3) | (cntx->dlci_id << 2)),
        (FRAME_TYPE_DM | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };    
    const uint8_t address       = ((cntx->role == ROLE_INITIATOR) ? 3 : 1) | (cntx->dlci_id << 2);        
    const uint8_t write_byte[4] = 
    {
        address, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };    

    /* Complete the request frame write and read the response frame. */
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte));
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte));
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: establishment rejected by peer
 * - self iniated open multiplexer
 * - issue DLCI establishment request
 * - receive DLCI establishment response: rejected by peer
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_rejected_by_peer)
{   
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait_rejected_by_peer;
    
    const dlci_establish_context_t context = {1, ROLE_INITIATOR};
    mock_wait->func_context                = &context;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX); 
    FileHandle *obj = NULL;    
    const int ret = mbed::Mux::dlci_establish(context.dlci_id, status, &obj);
    CHECK_EQUAL(ret, 3);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_REJECT, status);
    CHECK_EQUAL(obj, NULL);    
    CHECK(!MuxClient::is_dlci_establish_triggered());
}


/* Do successfull multiplexer peer iniated open.*/
void mux_peer_iniated_open(const uint8_t *rx_buf, uint8_t rx_buf_len, bool expected_mux_start_event_state)
{    
    const uint8_t write_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&write_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };

    peer_iniated_request_rx(&(rx_buf[0]), rx_buf_len, &(write_byte[0]));
    peer_iniated_response_tx(&(write_byte[1]),
                             (sizeof(write_byte) - sizeof(write_byte[0])),
                             NULL,
                             expected_mux_start_event_state,
                             MuxClient::is_mux_start_triggered);  
}


/*
 * TC - dlci establishment sequence, self initiated, role responder: successfull establishment
 * - peer iniated open multiplexer
 * - issue DLCI establishment request
 * - receive DLCI establishment response
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_role_responder_succes)
{   
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);

    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };        
    const bool expected_mux_start_event_state = true;
    mux_peer_iniated_open(&(read_byte[0]), sizeof(read_byte), expected_mux_start_event_state);

    dlci_self_iniated_establish(ROLE_RESPONDER, 1);   
}


/*
 * TC - mux start-up: multiplexer allready open
 * - mux open: self initiated
 * - issue START request
 * - return code: Operation not started, multiplexer control channel allready open.      
 */
TEST(MultiplexerOpenTestGroup, mux_open_allready_open)
{    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    /* Issue new start. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 0);    
    
    CHECK(!MuxClient::is_mux_start_triggered());                    
}


void mux_start_self_initated_sem_wait_rejected_by_peer(const void *)
{
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_DM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    const uint8_t write_byte[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte));
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte));
}


/*
 * TC - mux start-up sequence, self initiated: establishment rejected by peer
 * - issue START request
 * - receive START response: rejected by peer
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_rejected_by_peer)
{    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* 1st establishent: reject by peer. */

    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_start_self_initated_sem_wait_rejected_by_peer;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_REJECT);       
    CHECK(!MuxClient::is_mux_start_triggered());            
    
    /* 2nd establishent: success. */
    
    mux_self_iniated_open();
    CHECK(!MuxClient::is_mux_start_triggered());
}


/*
 * TC - mux start-up sequence, self initiated: write failure
 * - write request returns error code which is forwarded to the user
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_write_failure)
{   
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
    
    /* --- begin verify TX sequence --- */
    
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = (uint32_t)-1;    

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, -1);
    
    CHECK(!MuxClient::is_mux_start_triggered());                    
}


void mux_start_self_initated_sem_wait_timeout(const void *)
{
    /* Complete the frame write. */
    const uint8_t write_buf[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_buf[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_buf[0]), sizeof(write_buf));

    /* --- begin frame re-transmit sequence --- */

    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_TIMEOUT_GENERATE};
    uint8_t                                  counter       = RETRANSMIT_COUNT;
    do {    
        mock_t * mock_write = mock_free_get("write");
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
        mock_write->input_param[0].param        = (uint32_t)&write_byte;        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    

        /* Trigger timer timeout. */
        mbed::EventQueueMock::io_control(eq_io_control);
        
        /* Re-transmit the complete remaining part of the frame. */
        self_iniated_request_tx(&(write_buf[0]), sizeof(write_buf));
        
        --counter;
    } while (counter != 0);
    
    /* Trigger timer to finish the re-transmission cycle and the whole request. Release the semaphore blocking the call 
       thread. */
    mock_t * mock_release = mock_free_get("release");
    CHECK(mock_release != NULL);
    mock_release->return_value = osOK;    
    mbed::EventQueueMock::io_control(eq_io_control);   
}


/*
 * TC - mux start-up sequence, request timeout failure:
 * - send START request
 * - request timeout timer expires 
 * - generate timeout event to the user
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_timeout)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
    
    /* --- begin verify TX sequence --- */
    
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_sem_wait_timeout;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);
    
    CHECK(!MuxClient::is_mux_start_triggered());                
}


/*
 * TC - mux start-up sequence, 1st try:request timeout failure, 2nd try: success
 * - send 1st START request
 * - request timeout timer expires 
 * - generate timeout event to the user
 * - send 2nd START request
 * - success
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_success_after_timeout)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_sem_wait_timeout;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);
    
    CHECK(!MuxClient::is_mux_start_triggered());              
    
    // 2nd try - success.
    mux_self_iniated_open();
    CHECK(!MuxClient::is_mux_start_triggered());                        
}


/*
 * TC - mux start-up sequence, peer initiated:
 * - receive START request
 * - send START response
 * - generate completion event to the user/client
 */
TEST(MultiplexerOpenTestGroup, mux_open_peer_initiated)
{    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);

    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);    
 
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };    
    const bool expected_mux_start_event_state = true;
    mux_peer_iniated_open(&(read_byte[0]), sizeof(read_byte), expected_mux_start_event_state);
}


/* Do successfull peer iniated dlci establishment.*/
void dlci_peer_iniated_establish_accept(Role           role, 
                                        const uint8_t *rx_buf, 
                                        uint8_t        rx_buf_len,                                 
                                        uint8_t        dlci_id,
                                        bool           expected_dlci_establishment_event_state)
{       
    const uint8_t write_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        (((role == ROLE_INITIATOR) ? 1 : 3) | (dlci_id << 2)),
        (FRAME_TYPE_UA | PF_BIT),        
        fcs_calculate(&write_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };

    peer_iniated_request_rx(&(rx_buf[0]), rx_buf_len, &(write_byte[0]));
    peer_iniated_response_tx(&(write_byte[1]),
                             (sizeof(write_byte) - sizeof(write_byte[0])),
                             NULL,
                             expected_dlci_establishment_event_state,
                             MuxClient::is_dlci_establish_triggered); 
}


/*
 * TC - dlci establishment sequence, peer initiated, role initiator: successfull establishment
 * - self iniated open multiplexer
 * - receive: DLCI establishment request
 * - respond: DLCI establishment response
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_initiated_role_initiator_success)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();

    const Role role            = ROLE_INITIATOR;
    const uint8_t dlci_id      = 1;
    const uint8_t read_byte[4] = 
    {
        (((role == ROLE_INITIATOR) ? 1 : 3) | (dlci_id << 2)),
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };        
    
    const  bool expected_dlci_established_event_state = true;
    dlci_peer_iniated_establish_accept(role, 
                                       &(read_byte[0]),
                                       sizeof(read_byte),
                                       dlci_id,
                                       expected_dlci_established_event_state);
}


/*
 * TC - dlci establishment sequence, peer initiated, role initiator: DLCI id allready used
 * - self iniated open multiplexer
 * - receive: DLCI establishment request
 * - respond: DLCI establishment response
 * - receive: DLCI establishment request: same DLCI ID
 * - respond: DLCI establishment response
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_initiated_dlci_id_used)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();

    const Role role            = ROLE_INITIATOR;
    const uint8_t dlci_id      = 1;
    const uint8_t read_byte[4] = 
    {
        (((role == ROLE_INITIATOR) ? 1 : 3) | (dlci_id << 2)),
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };      
    
    /* 1st cycle. */
    bool expected_dlci_established_event_state = true;
    dlci_peer_iniated_establish_accept(role, 
                                       &(read_byte[0]),
                                       sizeof(read_byte),
                                       dlci_id,
                                       expected_dlci_established_event_state);   

    /* 2nd cycle. */
    expected_dlci_established_event_state = false;
    dlci_peer_iniated_establish_accept(role, 
                                       &(read_byte[0]),
                                       sizeof(read_byte),
                                       dlci_id,
                                       expected_dlci_established_event_state);
}


/* Reject peer iniated dlci establishment request.*/
void dlci_peer_iniated_establish_reject(uint8_t        address_field, 
                                        const uint8_t *rx_buf, 
                                        uint8_t        rx_buf_len)
{       
    const uint8_t write_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        address_field,
        (FRAME_TYPE_DM | PF_BIT),        
        fcs_calculate(&write_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };

    peer_iniated_request_rx(&(rx_buf[0]), rx_buf_len, &(write_byte[0]));
    const bool expected_dlci_establishment_event_state = false;
    peer_iniated_response_tx(&(write_byte[1]),
                             (sizeof(write_byte) - sizeof(write_byte[0])),
                             NULL,
                             expected_dlci_establishment_event_state,
                             MuxClient::is_dlci_establish_triggered);     
}


/*
 * TC - dlci establishment sequence, peer initiated, role initiator: all DLCI ids used
 * - self iniated open multiplexer
 * - establish DLCIs max amount: success
 * - receive DLCI establishment request
 * - responde with DM as max count reached
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_initiated_all_dlci_ids_used)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    uint8_t i                                  = MAX_DLCI_COUNT;
    uint8_t dlci_id                            = DLCI_ID_LOWER_BOUND;
    const Role role                            = ROLE_INITIATOR;
    bool expected_dlci_established_event_state = true;    
    uint8_t read_byte[4]                       = 
    {
        ~0,
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };        
    
    /* Consume all available DLCI IDs. */
    do {   
        read_byte[0] = 1 | (dlci_id << 2);
        dlci_peer_iniated_establish_accept(role,
                                           &(read_byte[0]),
                                           sizeof(read_byte),
                                           dlci_id,
                                           expected_dlci_established_event_state);   

        --i;
        ++dlci_id;
    } while (i != 0);

    read_byte[0]                          = 1 | (dlci_id << 2);
    dlci_peer_iniated_establish_reject(1u | (dlci_id << 2), &(read_byte[0]), sizeof(read_byte));  
}


/*
 * TC - dlci establishment sequence, peer initiated, multiplexer not open
 * - receive: DLCI establishment request
 * - respond: DM frame
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_iniated_mux_not_open)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
  
    const uint8_t dlci_id      = 1;
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        /* Peer assumes the role of initiator. */
        3u | (dlci_id << 2),
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };        
         
    dlci_peer_iniated_establish_reject(3u | (dlci_id << 2), &(read_byte[0]), sizeof(read_byte));    
}


/*
 * TC - dlci establishment sequence, peer initiated, role responder: successfull establishment
 * - peer iniated open multiplexer
 * - receive: DLCI establishment request
 * - respond: DLCI establishment response
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_initiated_role_responder_succes)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };
    const bool expected_mux_start_event_state = true;
    mux_peer_iniated_open(&(read_byte[0]), sizeof(read_byte), expected_mux_start_event_state);    

    const Role role              = ROLE_RESPONDER;
    const uint8_t dlci_id        = 1;
    const uint8_t read_byte_2[4] = 
    {
        (((role == ROLE_INITIATOR) ? 1 : 3) | (dlci_id << 2)),
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte_2[0], 2),
        FLAG_SEQUENCE_OCTET
    };        
    
    const bool expected_dlci_established_event_state = true;
    dlci_peer_iniated_establish_accept(role,
                                       &(read_byte_2[0]),
                                       sizeof(read_byte_2),
                                       dlci_id,
                                       expected_dlci_established_event_state);    
}


/*
 * TC - dlci establishment sequence, self initiated: dlci_id lower bound
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_iniated_id_lower_bound)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);
}


/*
 * TC - dlci establishment sequence, peer initiated: dlci_id lower bound
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_iniated_id_lower_bound)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();

    const Role role            = ROLE_INITIATOR;
    const uint8_t dlci_id      = DLCI_ID_LOWER_BOUND;
    const uint8_t read_byte[4] = 
    {
        (((role == ROLE_INITIATOR) ? 1 : 3) | (dlci_id << 2)),
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };        
    
    const  bool expected_dlci_established_event_state = true;
    dlci_peer_iniated_establish_accept(role,
                                       &(read_byte[0]),
                                       sizeof(read_byte),
                                       dlci_id,
                                       expected_dlci_established_event_state);   
}
    
    
/*
 * TC - dlci establishment sequence, self initiated: dlci_id upper bound
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_iniated_id_upper_bound)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_UPPER_BOUND);
    CHECK(!MuxClient::is_dlci_establish_triggered());                       
}


/*
 * TC - dlci establishment sequence, peer initiated: dlci_id lower bound
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_iniated_id_upper_bound)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();

    const Role role            = ROLE_INITIATOR;
    const uint8_t dlci_id      = DLCI_ID_UPPER_BOUND;
    const uint8_t read_byte[4] = 
    {
        (((role == ROLE_INITIATOR) ? 1 : 3) | (dlci_id << 2)),
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };        
    
    const  bool expected_dlci_established_event_state = true;
    dlci_peer_iniated_establish_accept(role,
                                       &(read_byte[0]),
                                       sizeof(read_byte),
                                       dlci_id,
                                       expected_dlci_established_event_state);   
}


/*
 * TC - dlci establishment sequence, self initiated: dlci_id out of bound
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_iniated_id_oob)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
   
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj = NULL;    
    int ret = mbed::Mux::dlci_establish((DLCI_ID_LOWER_BOUND - 1), status, &obj);
    CHECK_EQUAL(2, ret);
    CHECK_EQUAL(obj, NULL);
    ret = mbed::Mux::dlci_establish((DLCI_ID_UPPER_BOUND + 1), status, &obj);
    CHECK_EQUAL(2, ret);   
    CHECK_EQUAL(obj, NULL);    
}


/*
 * TC - dlci establishment sequence, peer initiated: dlci_id out of bound
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_iniated_id_oob)
{
    /* Both lower and upper bound OOO tests yield to DLCI ID 0 establishment, which is covered by
     * mux_open_peer_initiated TC. */ 
}


/*
 * TC - mux start-up sequence, peer initiated: multiplexer allready open
 * - 1st establishment
 * -- receive START request
 * -- send START response
 * -- generate completion event to the user/client
 * - 2nd establishment
 * -- receive START request
 * -- send START response
 * -- DO NOT generate completion event to the user/client
 */
TEST(MultiplexerOpenTestGroup, mux_open_peer_initiated_allready_open)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);

    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);    

    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };    

    /* 1st cycle. */
    bool expected_mux_start_event_state = true;
    mux_peer_iniated_open(&(read_byte[0]), sizeof(read_byte), expected_mux_start_event_state);    
    
    /* 2nd cycle. */
    expected_mux_start_event_state = false;
    mux_peer_iniated_open(&(read_byte[1]), 
                          (sizeof(read_byte) - sizeof(read_byte[0])),
                          expected_mux_start_event_state);
}


/* Multiplexer semaphore wait call from mux_open_simultaneous_self_iniated TC. */
void mux_open_simultaneous_self_iniated_sem_wait(const void *context)
{
    /* Generate peer mux START request, which is ignored by the implementation. */
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte), NULL);    
    
    /* Generate the remaining part of the mux START request. */
    const uint8_t write_byte[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte));
    
    
    /* Generate peer mux START response, which is accepted by the implementation. */
    const uint8_t read_byte_2[4] = 
    {
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_response_rx(&(read_byte_2[0]), sizeof(read_byte_2));
}


/*
 * TC - mux start-up sequence, self initiated: peer issues mux start-up request while self iniated is in progress
 * - send 1st byte of START request
 * - START request received completely from the peer -> ignored by the implementation
 * - send remainder of the START request
 * - START response received by the implementation
 */
TEST(MultiplexerOpenTestGroup, mux_open_simultaneous_self_iniated)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
    
    /* --- begin verify TX sequence --- */
    
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_open_simultaneous_self_iniated_sem_wait;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);
}


/* Multiplexer semaphore wait call from mux_open_simultaneous_self_iniated_full_frame TC. */
void mux_open_simultaneous_self_iniated_full_frame_sem_wait(const void *context)
{    
    /* Generate the remaining part of the mux START request. */
    const uint8_t write_byte[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte));
    
    /* Generate peer mux START request, which is ignored by the implementation. */
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };
       
    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte), NULL);    
   
    /* Generate peer mux START response, which is accepted by the implementation. */
    const uint8_t read_byte_2[4] = 
    {
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };

    self_iniated_response_rx(&(read_byte_2[0]), sizeof(read_byte_2));
}


/*
 * TC - mux start-up sequence, self initiated: peer issues mux start-up request while self iniated is in progress
 * - send complete START request
 * - START request received completely from the peer -> ignored by the implementation
 * - START response received by the implementation
 */
TEST(MultiplexerOpenTestGroup, mux_open_simultaneous_self_iniated_full_frame)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
    
    /* --- begin verify TX sequence --- */
    
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    const uint32_t write_byte               = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[0].param        = (uint32_t)&write_byte;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_open_simultaneous_self_iniated_full_frame_sem_wait;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 2);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);        
}

/*
 * TC - mux start-up sequence, peer initiated: peer issues mux start-up request while self iniated is in progress
 * - START request received completely from the peer 
 * - send 1st byte of START response
 * - issue mux open
 * - API returns dedicated error code to signal peer iniated open is allready in progress
 * - peer iniated mux open calllback called
 */
TEST(MultiplexerOpenTestGroup, mux_open_simultaneous_peer_iniated)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);

    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);    
 
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };   
    const uint8_t write_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&write_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };

    /* Generate peer iniated establishment and trigger TX of 1st response byte. */
    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte), &(write_byte[0]));    
    
    /* Start while peer iniated is in progress. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 1);
    
    /* Complete the existing peer iniated establishent cycle. */
    const bool expected_mux_start_event_state = true;    
    peer_iniated_response_tx(&(write_byte[1]),
                             (sizeof(write_byte) - sizeof(write_byte[0])),
                             NULL,
                             expected_mux_start_event_state,
                             MuxClient::is_mux_start_triggered);      
}


void mux_open_self_iniated_dm_tx_in_progress_sem_wait(const void *context)
{
    const uint8_t *dlci_id      = static_cast<const uint8_t*>(context);    
    const uint8_t write_byte[4] = 
    {
        3u | (*dlci_id << 2),
        (FRAME_TYPE_DM | PF_BIT),        
        fcs_calculate(&write_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };       
    const uint8_t new_write_byte = FLAG_SEQUENCE_OCTET;
    
    /* Finish the mux open establishment with success. */    
    peer_iniated_response_tx(&write_byte[0], sizeof(write_byte), &new_write_byte, false, NULL);    

    /* DM response completed, complete the mux start-up establishent. */
    const uint8_t read_byte[4] = 
    {
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        fcs_calculate(&read_byte[0], 2),
        FLAG_SEQUENCE_OCTET
    };    
    const uint8_t write_byte_2[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        fcs_calculate(&write_byte_2[0], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx(&(write_byte_2[0]), sizeof(write_byte_2));
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte));   
}


/*
 * TC - mux start-up sequence, self initiated with delay as DM frame TX is in progress: 
 * - peer sends a DISC command to DLCI 0 
 * - send 1st byte of DM response 
 * - issue mux open, will be put pending until ongoing DM response gets completed.
 * - DM response gets completed
 * - complete mux start-up establishment.
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_iniated_dm_tx_in_progress)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
  
    const uint8_t dlci_id      = 0;
    const uint8_t read_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        /* Peer assumes the role of initiator. */
        3u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };           

    /* Generate DISC from peer and trigger TX of 1st response byte of DM. */        
    const uint8_t write_byte = FLAG_SEQUENCE_OCTET;
    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte), &write_byte);

    /* Issue multiplexer start while DM is in progress. */
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_open_self_iniated_dm_tx_in_progress_sem_wait;    
    mock_wait->func_context = dlci_id;
    
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(2, ret);
    CHECK_EQUAL(Mux::MUX_ESTABLISH_SUCCESS, status);
}


} // namespace mbed
