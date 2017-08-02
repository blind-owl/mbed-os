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
    virtual void on_dlci_establish(FileHandle *obj, uint8_t dlci_id) {};
    virtual void event_receive() {};    
    
    inline void reset() {_is_mux_start_triggered = false;}
    inline bool is_mux_start_triggered() const {return _is_mux_start_triggered;}
    
    MuxClient() : _is_mux_start_triggered(false) {};
private:
    
    bool _is_mux_start_triggered;
};


void MuxClient::on_mux_start()
{
    _is_mux_start_triggered = true;
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

#define MUX_START_FRAME_LEN                 5u                          /* Length of the multiplexer start frame in 
                                                                           number of bytes. */
#define WRITE_LEN                           1u                          /* Length of single write call in number of 
                                                                           bytes. */  
#define READ_LEN                            1u                          /* Length of single read call in number of 
                                                                           bytes. */
#define FLAG_SEQUENCE_OCTET                 0x7Eu                       /* Flag field used in the advanced option mode. 
                                                                           */
#define ADDRESS_MUX_START_REQ_OCTET         0x03u                       /* Address field value of the start multiplexer 
                                                                           request frame. */
#define ADDRESS_MUX_START_RESP_OCTET        ADDRESS_MUX_START_REQ_OCTET /* Address field value of the start multiplexer 
                                                                           response frame. */
#define CONTROL_MUX_START_REQ_OCTET         0x3Fu                       /* Control field value of the start multiplexer 
                                                                           request frame. */
#define CONTROL_MUX_START_ACCEPT_RESP_OCTET 0x13u                       /* Control field value of the start multiplexer 
                                                                           response frame, peer accept. */
#define CONTROL_MUX_START_REJECT_RESP_OCTET 0x1Fu                       /* Control field value of the start multiplexer 
                                                                           response frame, peer reject. */
#define MUX_START_FRAME_FCS                 0xFCu                       /* FCS field value of the start multiplexer 
                                                                           request frame. */
#define T1_TIMER_VALUE                      300u                        /* T1 timer value. */
#define T1_TIMER_EVENT_ID                   1                           /* T1 timer event id. */
#define CRC_TABLE_LEN                       256u                        /* CRC table length in number of bytes. */
#define RETRANSMIT_COUNT                    3u                          /* Retransmission count for the tx frames 
                                                                           requiring a response. */

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
 * LOOP UNTIL COMPLETE START REQUEST FRAME WRITE DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call poll
 * - call write
 * - call call_in in the last iteration for T1 timer
 * - CALL RETURN 
 */
void mux_start_self_iniated_tx()
{
    const uint8_t write_byte[4] = 
    {
        ADDRESS_MUX_START_REQ_OCTET, 
        CONTROL_MUX_START_REQ_OCTET, 
        MUX_START_FRAME_FCS,
        FLAG_SEQUENCE_OCTET
    };

    /* write the complete start request frame. */
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
        mock_write->input_param[0].param        = write_byte[tx_count];        
        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        

        if (tx_count == (sizeof(write_byte) - 1)) {
            
            /* Start frame write sequence gets completed, now start T1 timer. */   
            
            mock_t * mock_call_in   = mock_free_get("call_in");    
            CHECK(mock_call_in != NULL);     
            mock_call_in->return_value = T1_TIMER_EVENT_ID;        
            mock_call_in->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_call_in->input_param[0].param        = T1_TIMER_VALUE;                  
        }
        
        mbed::EventQueueMock::io_control(eq_io_control);   
        
        ++tx_count;        
    } while (tx_count != sizeof(write_byte));       
}


/*
 * LOOP UNTIL COMPLETE START RESPONSE FRAME READ DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call poll
 * - call read
 * - call cancel in the last iteration to cancel T1 timer
 * - CALL RETURN 
 */
void mux_start_self_iniated_rx(uint8_t control_field)
{
    const uint8_t read_byte[] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_RESP_OCTET, 
        control_field, 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };    
      
    /* read the complete start response frame. */
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
        mock_read->output_param[0].param       = &(read_byte[rx_count]);
        mock_read->output_param[0].len         = sizeof(read_byte[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 1;  

        if (rx_count == (sizeof(read_byte) - 1)) {
            
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
    } while (rx_count != sizeof(read_byte));       
}


/* Multiplexer semaphore wait call from self initiated multiplexer open TC(s). */
void mux_start_self_initated_sem_wait(void *context)
{
    mux_start_self_iniated_tx();
    mux_start_self_iniated_rx(CONTROL_MUX_START_ACCEPT_RESP_OCTET);    
}


/* Do successfull multiplexer self iniated open.*/
void mux_self_iniated_open()
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
    mock_write->input_param[0].param        = FLAG_SEQUENCE_OCTET;        
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
    CHECK_EQUAL(ret, 1);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);    
}


/*
 * TC - mux start-up sequence, self initiated: successfull establishment
 * - issue START request
 * - receive START response
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_succes)
{    
    mux_self_iniated_open();
    CHECK(!mux_client.is_mux_start_triggered());                    
}


/*
 * TC - mux start-up: multiplexer allready open
 * - mux open: self initiated
 * - issue START request
 * - return code: Operation not started, multiplexer control channel allready open.      
 */
TEST(MultiplexerOpenTestGroup, mux_open_allready_open)
{    
    mux_self_iniated_open();
   
    /* Issue new start. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, 0);    
    
    CHECK(!mux_client.is_mux_start_triggered());                    
}


void mux_start_self_initated_sem_wait_rejected_by_peer(void *)
{
    mux_start_self_iniated_tx();
    mux_start_self_iniated_rx(CONTROL_MUX_START_REJECT_RESP_OCTET);
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
    
    /* --- begin verify TX sequence --- */
    
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
      
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = FLAG_SEQUENCE_OCTET;        
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
    CHECK_EQUAL(ret, 1);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_REJECT);    
    
    CHECK(!mux_client.is_mux_start_triggered());                    
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
    mock_write->input_param[0].param        = FLAG_SEQUENCE_OCTET;        
    mock_write->input_param[1].param        = WRITE_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = (uint32_t)-1;    

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const int ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(ret, -1);
    
    CHECK(!mux_client.is_mux_start_triggered());                    
}


void mux_start_self_initated_sem_wait_timeout(void *)
{
    /* Compelte the frame write. */
    mux_start_self_iniated_tx();

    /* --- begin frame re-transmit sequence --- */

    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_TIMEOUT_GENERATE};
    uint8_t                                  counter       = RETRANSMIT_COUNT;
    do {    
        mock_t * mock_write = mock_free_get("write");
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = FLAG_SEQUENCE_OCTET;        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    

        /* Trigger timer timeout. */
        mbed::EventQueueMock::io_control(eq_io_control);
        
        /* Re-transmit the complete remaining part of the frame. */
        mux_start_self_iniated_tx();
        
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
    mock_write->input_param[0].param        = FLAG_SEQUENCE_OCTET;        
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
    CHECK_EQUAL(ret, 1);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);
    
    CHECK(!mux_client.is_mux_start_triggered());                
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
    
    const uint8_t read_byte[] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        CONTROL_MUX_START_REQ_OCTET, 
        fcs_calculate(&read_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };    
      
    const uint8_t write_byte[5] = 
    {
        FLAG_SEQUENCE_OCTET,        
        ADDRESS_MUX_START_RESP_OCTET, 
        CONTROL_MUX_START_ACCEPT_RESP_OCTET, 
        fcs_calculate(&write_byte[1], 2),
        FLAG_SEQUENCE_OCTET
    };
    
    /* read the complete start request frame. */
    uint8_t                                  rx_count      = 0;       
    uint8_t                                  tx_count      = 0;          
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
        mock_read->output_param[0].param       = &(read_byte[rx_count]);
        mock_read->output_param[0].len         = sizeof(read_byte[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 1;  
       
        if (rx_count == (sizeof(read_byte) - 1)) {
            
            /* Start request frame read sequence gets completed, now begin the start response frame sequence. */   
            
            mock_t * mock_write = mock_free_get("write");
            CHECK(mock_write != NULL); 
            
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = write_byte[tx_count++];       
            
            mock_write->input_param[1].param        = WRITE_LEN;
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 1;
        }
        
        mbed::EventQueueMock::io_control(eq_io_control);   

        ++rx_count;        
    } while (rx_count != sizeof(read_byte));           
   
    /* write the remainder of the start response frame. */
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
        mock_write->input_param[0].param        = write_byte[tx_count];        
        
        mock_write->input_param[1].param        = WRITE_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        

        mbed::EventQueueMock::io_control(eq_io_control);  
        
        if (tx_count == (sizeof(write_byte) - 1)) {        
            
            /* Last byte of the start response frame written, verify completion callback. */            
            CHECK(mux_client.is_mux_start_triggered());
        } else {
            CHECK(!mux_client.is_mux_start_triggered());            
        }
        
        ++tx_count;        
    } while (tx_count != sizeof(write_byte));
}

// @todo: peer initiated: allready open
// @todo: simultaneous open


} // namespace mbed
