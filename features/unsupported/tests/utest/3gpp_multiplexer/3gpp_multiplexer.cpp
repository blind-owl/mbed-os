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


#define MAX_DLCI_COUNT 3u /* Max amount of DLCIs. */
static FileHandle* m_file_handle[MAX_DLCI_COUNT] = {NULL};

TEST_GROUP(MultiplexerOpenTestGroup)
{
    void setup()
    {
        for (uint8_t i = 0; i != sizeof(m_file_handle) / sizeof(m_file_handle[0]) ; ++i) {
            m_file_handle[i] = NULL;
        }
        
        mock_init();
        Mux::module_init();
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

#define FRAME_HEADER_READ_LEN        3u
#define FRAME_TRAILER_LEN            2u
#define FLAG_SEQUENCE_OCTET_LEN      1u                          /* Length of the flag sequence field in number of 
                                                                    bytes. */
#define SABM_FRAME_LEN               6u                          /* Length of the SABM frame in number of bytes. */
#define DM_FRAME_LEN                 6u                          /* Length of the DM frame in number of bytes. */
#define UA_FRAME_LEN                 6u                          /* Length of the UA frame in number of bytes. */
#define UIH_FRAME_LEN                7u                          /* Length of the minium UIH frame in number of bytes.*/
#define WRITE_LEN                    1u                          /* Length of single write call in number of bytes. */  
#define READ_LEN                     1u                          /* Length of single read call in number of bytes. */
#define FLAG_SEQUENCE_OCTET          0xF9u                       /* Flag field used in the basic option mode. */
#define ADDRESS_MUX_START_REQ_OCTET  0x03u                       /* Address field value of the start multiplexer 
                                                                    request frame. */
#define ADDRESS_MUX_START_RESP_OCTET ADDRESS_MUX_START_REQ_OCTET /* Address field value of the start multiplexer 
                                                                    response frame. */
#define LENGTH_INDICATOR_OCTET       1u                          /* Length indicator field value used in frame. */
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
#define TX_BUFFER_SIZE               31u                         /* Size of the TX buffer in number of bytes. */ 
#define RX_BUFFER_SIZE               TX_BUFFER_SIZE              /* Size of the RX buffer in number of bytes. */ 


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
 * - call read 
 * - call write
 * - call call_in in the last iteration for T1 timer
 * - CALL RETURN 
 */
void self_iniated_request_tx(const uint8_t *tx_buf, uint8_t tx_buf_len, uint8_t read_len)
{
    /* Write the complete request frame in the do...while. */
    uint8_t                                  tx_count      = 0;           
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};    
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};
    do {    
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;
        
        mbed::FileHandleMock::io_control(io_control);
        
        /* Nothing to read. */
        mock_t * mock_read = mock_free_get("read");
        CHECK(mock_read != NULL);
        mock_read->output_param[0].param       = NULL;
        mock_read->output_param[0].len         = 0;        
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = -EAGAIN;                         
        
        mock_t * mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);        
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(tx_buf[tx_count]);               
        mock_write->input_param[1].param        = tx_buf_len - tx_count;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        
       
        if (tx_count == tx_buf_len - 1) {            
            /* Start frame write sequence gets completed, now start T1 timer. */   
            
            mock_t * mock_call_in = mock_free_get("call_in");    
            CHECK(mock_call_in != NULL);     
            mock_call_in->return_value = T1_TIMER_EVENT_ID;        
            mock_call_in->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_call_in->input_param[0].param        = T1_TIMER_VALUE;                  
        } else {
            /* End the write cycle after successfull write made above in this loop. */
            
            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL);               
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(tx_buf[tx_count + 1u]);               
            mock_write->input_param[1].param        = tx_buf_len - (tx_count + 1u);
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 0;        
        }
        
        mbed::EventQueueMock::io_control(eq_io_control);   
        
        ++tx_count;        
    } while (tx_count != tx_buf_len);
}


typedef enum
{
    READ_FLAG_SEQUENCE_OCTET = 0, 
    SKIP_FLAG_SEQUENCE_OCTET
} FlagSequenceOctetReadType;


typedef enum
{
    STRIP_FLAG_FIELD_NO = 0, 
    STRIP_FLAG_FIELD_YES
} StripFlagFieldType;

/* Read complete response frame from the peer
 */
void self_iniated_response_rx(const uint8_t            *rx_buf, 
                              uint8_t                   rx_buf_len, // @todo remove me
                              const uint8_t            *write_byte,
                              FlagSequenceOctetReadType read_type,
                              StripFlagFieldType        strip_flag_field_type)
{
    /* Guard against internal logic error. */
    CHECK(!((read_type == READ_FLAG_SEQUENCE_OCTET) && (strip_flag_field_type == STRIP_FLAG_FIELD_YES)));
        
    uint8_t                                  rx_count      = 0;       
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    

    /* Enqueue deferred call to EventQueue.
     * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
    mock_t * mock = mock_free_get("call");
    CHECK(mock != NULL);           
    mock->return_value = 1;

    mbed::FileHandleMock::io_control(io_control);
           
    mock_t * mock_read;
    if (read_type == READ_FLAG_SEQUENCE_OCTET) {
        /* Phase 1: read frame start flag. */    
    
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = FLAG_SEQUENCE_OCTET_LEN;
        mock_read->return_value                = 1;   
        
        ++rx_count;
    }
           
    uint8_t read_len = FRAME_HEADER_READ_LEN;           
    if (strip_flag_field_type == STRIP_FLAG_FIELD_YES) {        
        /* Flag field present, which will be discarded by the implementation. */
        
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;
    }
    
    /* Phase 2: read next 3 bytes 1-byte at a time. */
    do {    
        /* Continue read cycle within current context. */            

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;    
        --read_len;
    } while (read_len != 0); 
    
    /* Phase 3: read trailing bytes after decoding length field 1-byte at a time. */
    read_len = FRAME_TRAILER_LEN;
    do {    
        /* Continue read cycle within current context. */            

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;    
        --read_len;
    } while (read_len != 0);     
    
    /* Frame read sequence gets completed, now cancel T1 timer. */              
    mock_t * mock_cancel = mock_free_get("cancel");    
    CHECK(mock_cancel != NULL);    
    mock_cancel->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_cancel->input_param[0].param        = T1_TIMER_EVENT_ID;     
            
    /* Release the semaphore blocking the call thread. */
    mock_t * mock_release = mock_free_get("release");
    CHECK(mock_release != NULL);
    mock_release->return_value = osOK;    

    /* Trigger the deferred call context to execute all mocks. */
    mbed::EventQueueMock::io_control(eq_io_control);               
}

#if 0
void self_iniated_response_rx(const uint8_t *rx_buf, uint8_t rx_buf_len, const uint8_t *write_byte)
{
    /* Read the complete response frame in do...while. */
    uint8_t                                  rx_count      = 0;       
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    do {
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;

        mbed::FileHandleMock::io_control(io_control);
        
        mock_t * mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 1;  
       
        if (rx_count == (rx_buf_len - 1)) {            
            /* Start frame read sequence gets completed, now cancel T1 timer with the RX decode. */   
            
            mock_t * mock_cancel = mock_free_get("cancel");    
            CHECK(mock_cancel != NULL);    
            mock_cancel->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_cancel->input_param[0].param        = T1_TIMER_EVENT_ID;     
            
            /* Release the semaphore blocking the call thread. */
            mock_t * mock_release = mock_free_get("release");
            CHECK(mock_release != NULL);
            mock_release->return_value = osOK;
            
            if (write_byte != NULL)  {
                /* RX frame gets completed start the pending frame TX sequence within the RX decode. */                 
                
                mock_t * mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 1;
                
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 0;
            }
        }
       
        /* End RX cycle. */
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL);
        mock_read->output_param[0].param       = NULL;
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 0;                 

        mbed::EventQueueMock::io_control(eq_io_control);   

        ++rx_count;        
    } while (rx_count != rx_buf_len);           
}
#endif // 0

/*
 * LOOP UNTIL COMPLETE REQUEST FRAME READ DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call read
 * - begin response frame TX sequence in the last iteration if parameter supplied
 * - call read
 * - CALL RETURN 
 */
void peer_iniated_request_rx(const uint8_t            *rx_buf, 
                             uint8_t                   rx_buf_len, 
                             FlagSequenceOctetReadType read_type,
                             const uint8_t            *resp_write_byte,
                             const uint8_t            *current_tx_write_byte,
                             uint8_t                   current_tx_write_byte_len)
{
    /* Internal logic error if both supplied params are != NULL. */
    CHECK(!((resp_write_byte != NULL) && (current_tx_write_byte != NULL)));
    
    /* Phase 1: read frame start flag. */
    uint8_t                                  rx_count      = 0;       
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    

    /* Enqueue deferred call to EventQueue.
     * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
    mock_t * mock = mock_free_get("call");
    CHECK(mock != NULL);           
    mock->return_value = 1;

    mbed::FileHandleMock::io_control(io_control);

    mock_t * mock_read;    
    if (read_type == READ_FLAG_SEQUENCE_OCTET) {
        /* Phase 1: read frame start flag. */    
    
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = FLAG_SEQUENCE_OCTET_LEN;
        mock_read->return_value                = 1;   
        
        ++rx_count;
    }    
           
    /* Phase 2: read next 3 bytes 1-byte at a time. */
    uint8_t read_len = FRAME_HEADER_READ_LEN;
    do {    
        /* Continue read cycle within current context. */            

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;    
        --read_len;
    } while (read_len != 0); 
    
    /* Phase 3: read trailing bytes after decoding length field 1-byte at a time. */
    read_len = FRAME_TRAILER_LEN;
    do {    
        /* Continue read cycle within current context. */            

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;    
        --read_len;
    } while (read_len != 0);
    
    mock_t * mock_write;
    if (resp_write_byte != NULL)  {
        /* RX frame completed, start the response frame TX sequence inside the current RX cycle. */ 
        
        const uint8_t length_of_frame = 4u + (resp_write_byte[3] & ~1) + 2u; // @todo: FIX ME: magic numbers.
                
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);                
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(resp_write_byte[0]);
        mock_write->input_param[1].param        = length_of_frame;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;
                
        /* End TX sequence: this call orginates from tx_internal_resp_entry_run(). */
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);                
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(resp_write_byte[1]);                       
        mock_write->input_param[1].param        = (length_of_frame - 1u);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 0;
                
        /* End TX sequence: this call orginates from on_deferred_call(). */
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);                
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(resp_write_byte[1]);                       
        mock_write->input_param[1].param        = (length_of_frame - 1u);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 0;
    } else if (current_tx_write_byte != NULL) {
        /* End TX sequence for the current byte in the TX pipeline: this call originates from on_deferred_call(). */

        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);               
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(current_tx_write_byte[0]);                       
        mock_write->input_param[1].param        = current_tx_write_byte_len;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 0;                                  
    } else {
        /* No implementation required. */
    }
   
    /* Trigger the deferred call context to execute all mocks. */
    mbed::EventQueueMock::io_control(eq_io_control);                   
}


#if 0
void peer_iniated_request_rx(const uint8_t *rx_buf, 
                             uint8_t        rx_buf_len, 
                             const uint8_t *resp_write_byte,
                             const uint8_t *current_tx_write_byte)
{
    /* Internal logic error if both supplied params are != NULL. */
    CHECK(!((resp_write_byte != NULL) && (current_tx_write_byte != NULL)));
        
    /* Read the complete request frame in do...while. */
    mock_t * mock_write;
    uint8_t                                  rx_count      = 0;      
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    do {
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;

        mbed::FileHandleMock::io_control(io_control);
        
        mock_t * mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 1; 
        
        if (resp_write_byte != NULL)  {
            if (rx_count == (rx_buf_len - 1)) {                
                /* RX frame gets completed start the response frame TX sequence within RX decode. */ 
                
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(resp_write_byte[0]);
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 1;
                
                /* End TX sequence: this call orginates from tx_internal_resp_entry_run(). */
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(resp_write_byte[1]);                       
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 0;
                
                /* End TX sequence: this call orginates from on_deferred_call(). */
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(resp_write_byte[1]);                       
                mock_write->input_param[1].param        = WRITE_LEN;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 0;             
            }
        } else if (current_tx_write_byte != NULL) {
            /* End TX sequence for the current byte in the TX pipeline: this call orginates from on_deferred_call(). */
            
            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL);               
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(current_tx_write_byte[0]);                       
            mock_write->input_param[1].param        = WRITE_LEN;
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 0;                                   
        } else {
            /* No implementation required. */
        }
        
        /* End RX cycle. */
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL);
        mock_read->output_param[0].param       = NULL;
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = READ_LEN;
        mock_read->return_value                = 0;                                 
        
        mbed::EventQueueMock::io_control(eq_io_control);   

        ++rx_count;        
    } while (rx_count != rx_buf_len);    
}
#endif // 0

/*
 * LOOP UNTIL COMPLETE REQUEST FRAME READ DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call read
 * - complete response frame TX in the last iteration if parameter supplied
 * - CALL RETURN 
 */
void peer_iniated_request_rx_full_frame_tx(const uint8_t *rx_buf, 
                                           uint8_t        rx_buf_len, 
                                           const uint8_t *write_byte,
                                           uint8_t        tx_buf_len)
{   
    uint8_t rx_count                                       = 0;               
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    

    /* Enqueue deferred call to EventQueue.
     * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
    mock_t * mock = mock_free_get("call");
    CHECK(mock != NULL);           
    mock->return_value = 1;

    mbed::FileHandleMock::io_control(io_control);

    /* Phase 1: read frame start flag. */    
    mock_t * mock_read = mock_free_get("read");
    CHECK(mock_read != NULL); 
    mock_read->output_param[0].param       = &(rx_buf[rx_count]);
    mock_read->output_param[0].len         = sizeof(rx_buf[0]);
    mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_read->input_param[0].param        = FLAG_SEQUENCE_OCTET_LEN;
    mock_read->return_value                = 1;   
        
    ++rx_count;
           
    /* Phase 2: read next 3 bytes 1-byte at a time. */
    uint8_t read_len = FRAME_HEADER_READ_LEN;
    do {    
        /* Continue read cycle within current context. */            

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;    
        --read_len;
    } while (read_len != 0); 
    
    /* Phase 3: read trailing bytes after decoding length field 1-byte at a time. */
    read_len = FRAME_TRAILER_LEN;
    do {    
        /* Continue read cycle within current context. */            

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(rx_buf[rx_count]);
        mock_read->output_param[0].len         = sizeof(rx_buf[0]);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = read_len;
        mock_read->return_value                = 1;          
        
        ++rx_count;    
        --read_len;
    } while (read_len != 0);

    /* RX frame completed, start the response frame TX sequence inside the current RX cycle. */
    mock_t * mock_write;        
    uint8_t i = 0;
    do {
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_byte[i]);        
        mock_write->input_param[1].param        = (tx_buf_len - i);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    
    
        ++i;
    } while (i != tx_buf_len);        
    
    /* Trigger the deferred call context to execute all mocks. */
    mbed::EventQueueMock::io_control(eq_io_control);
}


/*
 * LOOP UNTIL COMPLETE RESPONSE FRAME WRITE DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call read
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
         * Trigger sigio callback from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;
        
        mbed::FileHandleMock::io_control(io_control);

        /* Nothing to read. */
        mock_t * mock_read = mock_free_get("read");
        CHECK(mock_read != NULL);
        mock_read->output_param[0].param       = NULL;
        mock_read->output_param[0].len         = 0;        
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = FRAME_HEADER_READ_LEN;
        mock_read->return_value                = -EAGAIN;                                 
        
        mock_t * mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);        
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(buf[tx_count]);               
        mock_write->input_param[1].param        = (buf_len - tx_count);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        

        if (tx_count == (buf_len - 1)) {       
            if (new_tx_byte != NULL) {
                /* Last byte of the response frame written, write 1st byte of new pending frame. */    
                
                const uint8_t length_of_frame = 4u + (new_tx_byte[3] & ~1) + 2u; // @todo: FIX ME: magic numbers.    
                           
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(new_tx_byte[0]);                       
                mock_write->input_param[1].param        = length_of_frame;
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 1;
                
                /* End TX cycle. */                
                mock_write = mock_free_get("write");
                CHECK(mock_write != NULL);                
                mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->input_param[0].param        = (uint32_t)&(new_tx_byte[1]);                       
                mock_write->input_param[1].param        = (length_of_frame - 1u);
                mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
                mock_write->return_value                = 0;                                           
            }
        } else {
            /* End TX cycle. */
            
            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL);               
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(buf[tx_count + 1u]);               
            mock_write->input_param[1].param        = (buf_len - (tx_count + 1u));
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 0;        
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


/*
 * LOOP UNTIL COMPLETE RESPONSE FRAME WRITE DONE
 * - trigger sigio callback from FileHandleMock
 * - enqueue deferred call to EventQueue
 * - CALL RETURN 
 * - trigger deferred call from EventQueueMock
 * - call read
 * - call write
 * - release the pending call thread
 * - verify completion callback state in the last iteration, if supplied
 * - CALL RETURN 
 */
void peer_iniated_response_tx_no_pending_tx(const uint8_t *buf,
                                            uint8_t        buf_len,
                                            bool           expected_state,
                                            compare_func_t func)
{
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    uint8_t                                  tx_count      = 0;          
   
    /* Write the complete response frame in do...while. */
    do {   
        /* Enqueue deferred call to EventQueue. 
         * Trigger sigio callback event from the Filehandle used by the Mux (component under test). */
        mock_t * mock = mock_free_get("call");
        CHECK(mock != NULL);           
        mock->return_value = 1;
        
        mbed::FileHandleMock::io_control(io_control);

        /* Nothing to read. */
        mock_t * mock_read = mock_free_get("read");
        CHECK(mock_read != NULL);
        mock_read->output_param[0].param       = NULL;
        mock_read->output_param[0].len         = 0;        
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_read->input_param[0].param        = FRAME_HEADER_READ_LEN;
        mock_read->return_value                = -EAGAIN;                                 
        
        mock_t * mock_write = mock_free_get("write");
        CHECK(mock_write != NULL);        
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(buf[tx_count]);
        mock_write->input_param[1].param        = (buf_len - tx_count);;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;        

        if (tx_count == (buf_len - 1)) {      
            /* Release the pending call thread. */
            
            mock_t * mock_release = mock_free_get("release");
            CHECK(mock_release != NULL);
            mock_release->return_value = osOK;                        
        } else {
            /* End TX cycle. */
            
            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL);               
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(buf[tx_count + 1u]);               
            mock_write->input_param[1].param        = (buf_len - (tx_count + 1u));            
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 0;        
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


typedef struct 
{
    const uint8_t            *write_byte;
    uint8_t                   tx_cycle_read_len;
    FlagSequenceOctetReadType rx_cycle_read_type;
    StripFlagFieldType        strip_flag_field_type;
} mux_self_iniated_open_context_t;


/* Multiplexer semaphore wait call from self initiated multiplexer open TC(s). */
void mux_start_self_initated_sem_wait(const void *context)
{
    const uint8_t read_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };

    const mux_self_iniated_open_context_t *cntx = (const mux_self_iniated_open_context_t *)context;
    self_iniated_request_tx(cntx->write_byte, (SABM_FRAME_LEN - 1u), cntx->tx_cycle_read_len);
    self_iniated_response_rx(&(read_byte[0]), 
                             sizeof(read_byte), 
                             NULL,
                             cntx->rx_cycle_read_type,
                             cntx->strip_flag_field_type);   
}


/* Do successfull multiplexer self iniated open.*/
void mux_self_iniated_open(uint8_t                   tx_cycle_read_len, 
                           FlagSequenceOctetReadType rx_cycle_read_type,
                           StripFlagFieldType        strip_flag_field_type)
{     
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&write_byte[0];        
    mock_write->input_param[1].param        = SABM_FRAME_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;

    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&write_byte[1];                
    mock_write->input_param[1].param        = SABM_FRAME_LEN -1u;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_sem_wait;

    const mux_self_iniated_open_context_t context = {
        &(write_byte[1]),
        tx_cycle_read_len,
        rx_cycle_read_type,
        strip_flag_field_type
    };
    mock_wait->func_context = &context;    

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_SUCCESS, status);
}


void mux_self_iniated_open()
{
    mux_self_iniated_open(FLAG_SEQUENCE_OCTET_LEN, READ_FLAG_SEQUENCE_OCTET, STRIP_FLAG_FIELD_NO);    
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
}


/* Multiplexer semaphore wait call from mux_open_self_initiated_existing_open_pending TC. */
void mux_start_self_initated_existing_open_pending_sem_wait(const void *context)
{
    /* Issue new self iniated mux open, which fails. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const uint32_t ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(ret, 1);
    
    /* Finish the mux open establishment with success. */
    mux_start_self_initated_sem_wait(context);
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
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };
    
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
    mock_write->input_param[0].param        = (uint32_t)&write_byte[0];        
    mock_write->input_param[1].param        = SABM_FRAME_LEN;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&write_byte[1];        
    mock_write->input_param[1].param        = SABM_FRAME_LEN - 1u;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;            

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_start_self_initated_existing_open_pending_sem_wait;

    const mux_self_iniated_open_context_t context = {
        &(write_byte[1]),
        FLAG_SEQUENCE_OCTET_LEN,
        READ_FLAG_SEQUENCE_OCTET
    };
    mock_wait->func_context = &context;   
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);       
    
    /* Issue new self iniated mux open, which fails. */
    ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_NO_RESOURCE, ret);    
}


/* Multiplexer semaphore wait call from mux_open_self_initiated_existing_open_pending_2 TC. */
void mux_start_self_initated_existing_open_pending_2_sem_wait(const void *context)
{
    /* Issue new self iniated mux open, which fails. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const uint32_t ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(ret, 1);

    const uint8_t write_byte_2[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte_2[1], 3),
        FLAG_SEQUENCE_OCTET
    };    
    
    /* Finish the TX of DM response. */   
    peer_iniated_response_tx((uint8_t *)context, (DM_FRAME_LEN -1u), &(write_byte_2[0]), false, NULL);
    
    /* DM response completed, complete the mux start-up establishment. */
    const uint8_t read_byte[5] = 
    {
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3),
        FLAG_SEQUENCE_OCTET
    };   
    self_iniated_request_tx(&(write_byte_2[1]), 
                            (sizeof(write_byte_2) - sizeof(write_byte_2[0])),
                            FRAME_HEADER_READ_LEN);
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte), NULL, SKIP_FLAG_SEQUENCE_OCTET, STRIP_FLAG_FIELD_NO);
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
    const uint8_t read_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,        
        /* Peer assumes the role of initiator. */
        3u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };           

    /* Generate DISC from peer and trigger TX of DM response. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),
        (FRAME_TYPE_DM | PF_BIT),    
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3),
        FLAG_SEQUENCE_OCTET        
    };
    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte),READ_FLAG_SEQUENCE_OCTET, &(write_byte[0]), NULL, 0);   
    
    /* Issue multiplexer start while DM is in progress. */
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_existing_open_pending_2_sem_wait;    
    mock_wait->func_context = &(write_byte[1]);
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);
    
    /* Issue new self iniated mux open, which fails. */
    ret = mbed::Mux::mux_start(status);   
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_NO_RESOURCE, ret);    
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
    
    const uint8_t read_byte[5]  = 
    {
        (((cntx->role == ROLE_INITIATOR) ? 1 : 3) | (cntx->dlci_id << 2)),
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,        
        fcs_calculate(&read_byte[0], 3),
        FLAG_SEQUENCE_OCTET
    };    
    const uint8_t address       = ((cntx->role == ROLE_INITIATOR) ? 3 : 1) | (cntx->dlci_id << 2);    
    const uint8_t write_byte[5] = 
    {
        address, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,        
        fcs_calculate(&write_byte[0], 3),
        FLAG_SEQUENCE_OCTET
    };    

    /* Complete the request frame write and read the response frame. */
    self_iniated_request_tx(&(write_byte[0]), sizeof(write_byte), FRAME_HEADER_READ_LEN);
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte), NULL, SKIP_FLAG_SEQUENCE_OCTET, STRIP_FLAG_FIELD_NO);
}


/* Do successfull self iniated dlci establishment.*/
FileHandle* dlci_self_iniated_establish(Role role, uint8_t dlci_id)
{
    const uint32_t address      = ((role == ROLE_INITIATOR) ? 3u : 1u) | (dlci_id << 2);        
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        address, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,        
        fcs_calculate(&write_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };    
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;

    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait;
    
    const dlci_establish_context_t context = {dlci_id, role};
    mock_wait->func_context                = &context;

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj                      = NULL;
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_SUCCESS);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);      
    CHECK(obj != NULL);
    
    return obj;
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
    FileHandle *obj                      = NULL;    
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(1, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_MUX_NOT_OPEN);
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
    /* Complete the request frame write. */
    const uint8_t *write_buf = (const uint8_t *)context;
    self_iniated_request_tx(&(write_buf[1]), (SABM_FRAME_LEN - 1u), FRAME_HEADER_READ_LEN);
    
    /* --- begin frame re-transmit sequence --- */

    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_TIMEOUT_GENERATE};
    uint8_t                                  counter       = RETRANSMIT_COUNT;
    do {    
        mock_t * mock_write = mock_free_get("write");
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_buf[0]);
        mock_write->input_param[1].param        = SABM_FRAME_LEN;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    
        
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&write_buf[1];        
        mock_write->input_param[1].param        = (SABM_FRAME_LEN - 1u);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 0;                

        /* Trigger timer timeout. */
        mbed::EventQueueMock::io_control(eq_io_control);
        
        /* Re-transmit the complete remaining part of the frame. */
        self_iniated_request_tx(&(write_buf[1]), (SABM_FRAME_LEN - 1u), FRAME_HEADER_READ_LEN);
        
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
 * - 2nd iteration will succeed
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
    
    const uint8_t write_buf[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        (3u | (1u << 2)),
        (FRAME_TYPE_SABM | PF_BIT),
        LENGTH_INDICATOR_OCTET,                
        fcs_calculate(&write_buf[1], 3),
        FLAG_SEQUENCE_OCTET
    };
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_buf[0]);        
    mock_write->input_param[1].param        = sizeof(write_buf);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait_timeout;
    mock_wait->func_context = &(write_buf[0]);
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_buf[1]);                
    mock_write->input_param[1].param        = sizeof(write_buf) - sizeof(write_buf[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;                            

    /* Start test sequence. fails with timeout. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj                      = NULL;        
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(1u, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_SUCCESS);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);           
    CHECK_EQUAL(obj, NULL);
    
    /* 2nd iteration will succeed. */
    dlci_self_iniated_establish(ROLE_INITIATOR, 1);    
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
    
    const uint8_t write_buf[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        (3u | (1u << 2)),
        (FRAME_TYPE_SABM | PF_BIT),
        LENGTH_INDICATOR_OCTET,                
        fcs_calculate(&write_buf[1], 3),
        FLAG_SEQUENCE_OCTET
    };    
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_buf[0]);        
    mock_write->input_param[1].param        = sizeof(write_buf);    
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait_timeout;
    mock_wait->func_context = &(write_buf[0]);    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_buf[1]);                
    mock_write->input_param[1].param        = sizeof(write_buf) - sizeof(write_buf[0]);    
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;                            
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj                      = NULL;        
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(1u, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_SUCCESS);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);           
    CHECK_EQUAL(obj, NULL);
    
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
    FileHandle *obj                      = NULL;
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_NO_RESOURCE);    
    CHECK_EQUAL(obj, NULL);
}


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
    
    FileHandle* f_handle;
    uint8_t i       = MAX_DLCI_COUNT;
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    do {
        f_handle = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);
        CHECK(f_handle != NULL);
       
        --i;
        ++dlci_id;
    } while (i != 0);
    
    /* All available DLCI ids consumed. Next request will fail. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    FileHandle *obj                      = NULL;
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_NO_RESOURCE);    
    CHECK_EQUAL(obj, NULL);    
}


bool is_file_handle_uniqueue(FileHandle* obj, uint8_t current_idx)
{
    if (current_idx == 0) {
        return true;
    }
    
    for (uint8_t i = 0; i != current_idx; ++i) {
        if (m_file_handle[i] == obj) {
            return false;
        }
    }        
    
    m_file_handle[current_idx] = obj;
    
    return true;
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: all DLCI ids used
 * Cloned from TC: dlci_establish_self_initiated_all_dlci_ids_used
 * 
 * Difference: ensures established DLCIs are uniqueue
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_consume_all_dlci_ids_ensure_uniqueue)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();

    FileHandle *obj;
    bool bool_check;
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    for (uint8_t i = 0; i != MAX_DLCI_COUNT; ++i) {
        obj = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);
        CHECK(obj != NULL);
        bool_check = is_file_handle_uniqueue(obj, i);
        CHECK(bool_check);
        m_file_handle[i] = obj;        

        ++dlci_id;        
    }

    /* All available DLCI ids consumed. Next request will fail. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    obj = NULL;
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_NO_RESOURCE);    
    CHECK_EQUAL(obj, NULL);        
}


/* Multiplexer semaphore wait call from dlci_establish_self_initiated_rejected_by_peer TC. 
 * Role: initiator
 */
void dlci_establish_self_initated_sem_wait_rejected_by_peer(const void *context)
{    
    const uint8_t read_byte[5] = 
    {
        1u | (1u << 2),        
        (FRAME_TYPE_DM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,        
        fcs_calculate(&read_byte[0], 3),
        FLAG_SEQUENCE_OCTET
    };    

    /* Complete the request frame write and read the response frame. */
    const uint8_t *write_byte = (const uint8_t *)context;
    self_iniated_request_tx(&(write_byte[0]), (SABM_FRAME_LEN - 1u), FRAME_HEADER_READ_LEN);
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte), NULL, SKIP_FLAG_SEQUENCE_OCTET, STRIP_FLAG_FIELD_NO);
}


/*
 * TC - dlci establishment sequence, self initiated, role initiator: establishment rejected by peer
 * - self iniated open multiplexer
 * - issue DLCI establishment request
 * - receive DLCI establishment response: rejected by peer
 * - issue new DLCI establishment request: success
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
    
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (1u << 2), 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };    
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = SABM_FRAME_LEN;    
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_sem_wait_rejected_by_peer;
    mock_wait->func_context                = &(write_byte[1]);
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = (SABM_FRAME_LEN - 1u);        
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;                    

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX); 
    FileHandle *obj                      = NULL;    
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(1u, status, &obj);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_REJECT, status);
    CHECK_EQUAL(obj, NULL);
    
    /* 2nd time: establishment success. */
    dlci_self_iniated_establish(ROLE_INITIATOR, 1);
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
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_NO_RESOURCE, ret);
}


void mux_start_self_initated_sem_wait_rejected_by_peer(const void *context)
{
    const uint8_t read_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_DM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };
    
    self_iniated_request_tx((const uint8_t*)(context), (SABM_FRAME_LEN - 1u), FLAG_SEQUENCE_OCTET_LEN);
    self_iniated_response_rx(&(read_byte[0]), sizeof(read_byte), NULL, READ_FLAG_SEQUENCE_OCTET, STRIP_FLAG_FIELD_NO);
}


void mux_self_iniated_open_rx_frame_sync_done()
{
    mux_self_iniated_open(FRAME_HEADER_READ_LEN, SKIP_FLAG_SEQUENCE_OCTET, STRIP_FLAG_FIELD_YES);    
}


/*
 * TC - mux start-up sequence, self initiated: establishment rejected by peer
 * - issue START request
 * - receive START response: rejected by peer
 * - issue 2nd START request: establishment success
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
      
    /* 1st establishment: reject by peer. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);    
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);        
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value = 0;        

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_sem_wait_rejected_by_peer;
    mock_wait->func_context = &(write_byte[1]);

    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_REJECT, status);
    
    /* 2nd establishment: success. */
    mux_self_iniated_open_rx_frame_sync_done();
}


void mux_start_self_initated_sem_wait_timeout(const void *context)
{
    /* Complete the frame write. */    
    const uint8_t *write_buf = (const uint8_t *)context;
    self_iniated_request_tx(&(write_buf[1]), (SABM_FRAME_LEN - 1u), FLAG_SEQUENCE_OCTET_LEN);

    /* --- begin frame re-transmit sequence --- */

    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_TIMEOUT_GENERATE};
    uint8_t                                  counter       = RETRANSMIT_COUNT;
    do {    
        mock_t * mock_write = mock_free_get("write");
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_buf[0]);        
        mock_write->input_param[1].param        = SABM_FRAME_LEN;    
        
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    
        
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_buf[1]);        
        mock_write->input_param[1].param        = SABM_FRAME_LEN - 1u;            
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 0;                

        /* Trigger timer timeout. */
        mbed::EventQueueMock::io_control(eq_io_control);
        
        /* Re-transmit the complete remaining part of the frame. */
        self_iniated_request_tx(&(write_buf[1]), (SABM_FRAME_LEN - 1u), FLAG_SEQUENCE_OCTET_LEN);
        
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
 * TC - mux start-up sequence, 1st try:request timeout failure, 2nd try: success
 * - send START request
 * - request timeout timer expires 
 * - generate timeout event to the user
 * - 2nd mux start-up sequence will succeed
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
      
    const uint8_t write_buf[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_buf[1], 3),
        FLAG_SEQUENCE_OCTET
    };
    
    /* Set mock. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_buf[0]);        
    mock_write->input_param[1].param        = sizeof(write_buf);    
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_buf[1]);        
    mock_write->input_param[1].param        = sizeof(write_buf) - sizeof(write_buf[0]);        
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;                

    /* Set mock. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_start_self_initated_sem_wait_timeout;
    mock_wait->func_context = &(write_buf[0]);

    /* Start test sequence: fails with timeout. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_TIMEOUT);   
    
    /* Start test sequence: successfull. */
    mux_self_iniated_open();
}


/*
 * TC - mux start-up sequence, peer initiated:
 * - receive START request
 * Expected result:
 * - No action taken by the implementation
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
 
    const uint8_t read_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET,
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3),
        FLAG_SEQUENCE_OCTET
    };    

    peer_iniated_request_rx(&(read_byte[0]),sizeof(read_byte), READ_FLAG_SEQUENCE_OCTET, NULL, NULL, 0);
}


/*
 * TC - DLCI establish, peer initiated:
 * - receive DLCI START request from the peer
 * Expected result:
 * - No action taken by the implementation
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_peer_initiated)
{ 
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);

    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);    
    
    mux_self_iniated_open();    
 
    const uint8_t read_byte[5] = 
    {        
        1u | (DLCI_ID_LOWER_BOUND << 2),
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3),
        FLAG_SEQUENCE_OCTET
    };    

    peer_iniated_request_rx(&(read_byte[0]), sizeof(read_byte), SKIP_FLAG_SEQUENCE_OCTET, NULL, NULL, 0);
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
    uint32_t ret    = mbed::Mux::dlci_establish((DLCI_ID_LOWER_BOUND - 1u), status, &obj);
    CHECK_EQUAL(2, ret);
    CHECK_EQUAL(obj, NULL);
    ret = mbed::Mux::dlci_establish((DLCI_ID_UPPER_BOUND + 1u), status, &obj);
    CHECK_EQUAL(2, ret);   
    CHECK_EQUAL(obj, NULL);    
}


void mux_open_self_iniated_dm_tx_in_progress_sem_wait(const void *context)
{
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    }; 
    /* Finish the DM TX sequence and TX 1st byte of the pending mux start-up request. */   
    const uint8_t* write_byte_dm = (const uint8_t*)context;
    peer_iniated_response_tx(&write_byte_dm[0], (DM_FRAME_LEN - 1u), &write_byte[0], false, NULL);    

    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    uint32_t ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(1, ret);

    /* TX remainder of the mux start-up request. */
    self_iniated_request_tx(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), FRAME_HEADER_READ_LEN);    
       
    status = mbed::Mux::MUX_ESTABLISH_MAX;    
    ret    = mbed::Mux::mux_start(status);
    CHECK_EQUAL(1, ret);       
    
    /* RX mux start-up response. */
    const uint8_t read_byte[5] = 
    {
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };   
    self_iniated_response_rx(&(read_byte[0]), 
                             sizeof(read_byte), 
                             NULL,
                             SKIP_FLAG_SEQUENCE_OCTET,
                             STRIP_FLAG_FIELD_NO);
}


/*
 * TC - mux start-up sequence, self initiated with delay as DM frame TX is in progress: 
 * - peer sends a DISC command to DLCI 0 
 * - send 1st byte of DM response 
 * - issue mux open, will be put pending until ongoing DM response gets completed.
 * - DM response gets completed
 * - TX 1st byte of mux start-up request
 * - issue mux open: fails
 * - TX remainder of mux start-up request
 * - issue mux open: fails
 * - receive mux start-up response: establishment success
 * - issue mux open: fails
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
    const uint8_t read_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,        
        /* Peer assumes the role of initiator. */
        3u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };           

    /* Generate DISC from peer and trigger TX of response DM. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),
        (FRAME_TYPE_DM | PF_BIT),
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };   
    peer_iniated_request_rx(&(read_byte[0]), 
                            sizeof(read_byte), 
                            READ_FLAG_SEQUENCE_OCTET,                            
                            &(write_byte[0]),   // TX response frame within the RX cycle.
                            NULL,               // No current frame in the TX pipeline.
                            0);                                

    /* Issue multiplexer start while DM is in progress. */
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = mux_open_self_iniated_dm_tx_in_progress_sem_wait;   
    mock_wait->func_context = &(write_byte[1]);    

    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);   
    mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(Mux::MUX_ESTABLISH_SUCCESS, status);
    
    status = mbed::Mux::MUX_ESTABLISH_MAX;    
    ret    = mbed::Mux::mux_start(status);
    CHECK_EQUAL(Mux::MUX_STATUS_NO_RESOURCE, ret);
}


void dlci_establish_self_initated_dm_tx_in_progress_sem_wait(const void *context)
{
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (1u << 2),         
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };
    /* Finish the DM TX sequence and TX 1st byte of the pending DLCI establishment request. */   
    const uint8_t* write_byte_dm = (const uint8_t*)context;
    peer_iniated_response_tx(&write_byte_dm[0], (DM_FRAME_LEN - 1u), &write_byte[0], false, NULL);    

    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj                      = NULL;
    mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(1u, status, &obj);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_INPROGRESS, ret);
    CHECK_EQUAL(NULL, obj);
    
    /* TX remainder of the pending DLCI establishment request. */
    self_iniated_request_tx(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), FRAME_HEADER_READ_LEN);
    
    obj = NULL;
    ret = mbed::Mux::dlci_establish(1u, status, &obj);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_INPROGRESS, ret);
    CHECK_EQUAL(NULL, obj);

    /* RX DLCI establishment response. */
    const uint8_t read_byte[5] = 
    {
        3u | (1u << 2),         
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };
    self_iniated_response_rx(&(read_byte[0]), 
                             sizeof(read_byte), 
                             NULL,
                             SKIP_FLAG_SEQUENCE_OCTET,
                             STRIP_FLAG_FIELD_NO);        
}


/*
 * TC - DLCI establishment sequence, self initiated with delay as DM frame TX is in progress: 
 * - peer sends a DISC command to DLCI 1 
 * - send 1st byte of DM response 
 * - issue DLCI establish, will be put pending until ongoing DM response gets completed.
 * - DM response gets completed
 * - TX 1st byte of DLCI establish request
 * - issue DLCI establish: fails
 * - TX remainder of DLCI establish request
 * - issue mux open: fails
 * - receive DLCI establish response: establishment success
 * - issue DLCI establishment request with the same DLCI ID: fails as DLCI ID allready in use.
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_iniated_dm_tx_in_progress)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
  
    const uint8_t dlci_id      = 1u;
    const uint8_t read_byte[5] = 
    {
        /* Peer assumes the role of the responder. */
        1u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[/*1*/0], 3u),
        FLAG_SEQUENCE_OCTET
    };           

    /* Generate DISC from peer and trigger TX of DM response. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        1u | (dlci_id << 2), 
        (FRAME_TYPE_DM | PF_BIT),        
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };          
    peer_iniated_request_rx(&(read_byte[0]), 
                            sizeof(read_byte), 
                            SKIP_FLAG_SEQUENCE_OCTET,                            
                            &(write_byte[0]),   // TX response frame within the RX cycle.
                            NULL,               // No current frame in the TX pipeline.
                            0);                                

    /* Issue DLCI establishment while DM is in progress. */
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initated_dm_tx_in_progress_sem_wait;   
    mock_wait->func_context = &(write_byte[1]);    

    /* Start test sequence. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj                      = NULL;
    mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_SUCCESS, status);      
    CHECK(obj != NULL);
    
    /* Fails as DLCI ID allready in use. */
    obj = NULL;
    ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_NO_RESOURCE, ret);
    CHECK_EQUAL(NULL, obj);
}


/*
 * TC - Mux open: Peer sends DISC command to DLCI 0, which is ignored by the implementation.
 */
TEST(MultiplexerOpenTestGroup, mux_open_rx_disc_dlci_0)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
  
    const uint8_t dlci_id      = 0;
    const uint8_t read_byte[5] = 
    {
        /* Peer assumes the role of the responder. */
        1u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    
    /* Generate DISC from peer which is ignored buy the implementation. */       
    peer_iniated_request_rx(&(read_byte[0]), 
                            sizeof(read_byte), 
                            SKIP_FLAG_SEQUENCE_OCTET,                            
                            NULL,   // No TX response frame within the RX cycle.
                            NULL,   // No current frame in the TX pipeline.
                            0);                                
}


/*
 * TC - Peer sends DISC command to established(used) DLCI, which is ignored by the implementation.
 */
TEST(MultiplexerOpenTestGroup, mux_open_rx_disc_dlci_in_use)
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

    const uint8_t read_byte[5] = 
    {
        /* Peer assumes the role of the responder. */
        1u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };                     
    /* Generate DISC from peer which is ignored buy the implementation. */       
    peer_iniated_request_rx(&(read_byte[0]), 
                            sizeof(read_byte), 
                            SKIP_FLAG_SEQUENCE_OCTET,                            
                            NULL,   // No TX response frame within the RX cycle.
                            NULL,   // No current frame in the TX pipeline.
                            0);                                
}


/* Semaphore wait call from mux_open_self_initiated_full_frame_write_in_loop_succes TC. */
void mux_open_self_initiated_full_frame_write_in_loop_succes_sem_wait(const void *context)
{
    /* Program read cycle. */
    const uint8_t read_byte[6] =
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_RESP_OCTET, 
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };   
    self_iniated_response_rx(&(read_byte[0]), 
                             sizeof(read_byte), 
                             NULL,
                             READ_FLAG_SEQUENCE_OCTET,
                             STRIP_FLAG_FIELD_NO);                
}


/*
 * TC - multiplexer successfull open: request frame is written completely in 1 loop in the call context.
 */
TEST(MultiplexerOpenTestGroup, mux_open_self_initiated_full_frame_write_in_loop_succes)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    /* Program write cycle. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        ADDRESS_MUX_START_REQ_OCTET, 
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };       
    mock_t * mock_write;
    uint8_t i = 0;
    do {
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_byte[i]);        
        mock_write->input_param[1].param        = (SABM_FRAME_LEN - i);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    
    
        ++i;
    } while (i != sizeof(write_byte));

    /* Start frame write sequence gets completed, now start T1 timer. */              
    mock_t * mock_call_in = mock_free_get("call_in");    
    CHECK(mock_call_in != NULL);     
    mock_call_in->return_value                = T1_TIMER_EVENT_ID;        
    mock_call_in->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_call_in->input_param[0].param        = T1_TIMER_VALUE;         
            
    /* Set wait. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func = mux_open_self_initiated_full_frame_write_in_loop_succes_sem_wait;
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::mux_start(status);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, ret);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_SUCCESS, status);   
}


/* Semaphore wait call from dlci_establish_self_initiated_full_frame_write_in_loop_succes TC. */
void dlci_establish_self_initiated_full_frame_write_in_loop_succes_sem_wait(const void *context)
{
    /* Program read cycle. */   
    const uint8_t read_byte[5] = 
    {
        3u | (1u << 2),
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };   
    self_iniated_response_rx(&(read_byte[0]), 
                             sizeof(read_byte), 
                             NULL,
                             SKIP_FLAG_SEQUENCE_OCTET,
                             STRIP_FLAG_FIELD_NO);                
}


/*
 * TC - DLCI establishment successfull open: DLCI establishment request frame is written completely in 1 loop in the 
 * call context.
 */
TEST(MultiplexerOpenTestGroup, dlci_establish_self_initiated_full_frame_write_in_loop_succes)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();    

    /* Program write cycle. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (1u << 2),        
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };           
    mock_t * mock_write;
    uint8_t i = 0;
    do {
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_byte[i]);        
        mock_write->input_param[1].param        = (SABM_FRAME_LEN - i);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = 1;    
    
        ++i;
    } while (i != sizeof(write_byte));    
    
    /* Start frame write sequence gets completed, now start T1 timer. */              
    mock_t * mock_call_in = mock_free_get("call_in");    
    CHECK(mock_call_in != NULL);     
    mock_call_in->return_value                = T1_TIMER_EVENT_ID;        
    mock_call_in->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_call_in->input_param[0].param        = T1_TIMER_VALUE;         
            
    /* Set wait. */    
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = dlci_establish_self_initiated_full_frame_write_in_loop_succes_sem_wait;    
    
    /* Start test sequence. Test set mocks. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj                      = NULL;
    const mbed::Mux::MuxReturnStatus ret = mbed::Mux::dlci_establish(1u, status, &obj);
    CHECK_EQUAL(ret, mbed::Mux::MUX_STATUS_SUCCESS);
    CHECK_EQUAL(status, mbed::Mux::MUX_ESTABLISH_SUCCESS);      
    CHECK(obj != NULL);
}


/*
 * TC - DM response frame written completely in 1 loop in the call context 
 * - peer sends a DISC command to DLCI 0 
 * - send DM response completely in a single loop
 */
TEST(MultiplexerOpenTestGroup, mux_not_open_dm_tx_full_frame_write_in_loop)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
  
    const uint8_t dlci_id = 0;
    
    /* Program read cycle. */
    const uint8_t read_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,        
        /* Peer assumes the role of initiator. */
        3u | (dlci_id << 2),
        (FRAME_TYPE_DISC | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    
    /* Program write cycle. */
    const uint8_t write_byte[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        (FRAME_TYPE_DM | PF_BIT),         
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };           
    
    peer_iniated_request_rx_full_frame_tx(&(read_byte[0]), sizeof(read_byte), &(write_byte[0]), sizeof(write_byte));
}


static void user_tx_0_length_user_payload_callback()
{
    FAIL("TC FAILURE IF CALLED");    
}


/*
 * TC - Ensure that No Tx cycle is started when 0 leng write is issued
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Issue 0 length write request
 *
 * Expected outcome:
 * - No Tx is started
 * - No callback called
 */
TEST(MultiplexerOpenTestGroup, user_tx_0_length_user_payload)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1u;
    FileHandle* f_handle  = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);   
    f_handle->sigio(user_tx_0_length_user_payload_callback);
        
    const uint8_t write_dummy = 0xA5u;    
    const ssize_t ret         = f_handle->write(&write_dummy, 0);
    CHECK_EQUAL(0, ret);    
}


static void user_tx_size_lower_bound_tx_callback()
{
    FAIL("TC FAILURE IF CALLED");
}


/*
 * TC - 1 byte length UIH frame TX in 1 write call: lower bound test
 */
TEST(MultiplexerOpenTestGroup, user_tx_size_lower_bound)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1u;
    FileHandle* f_handle  = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);   
    f_handle->sigio(user_tx_size_lower_bound_tx_callback);
    
    /* Program write cycle. */
    uint8_t user_data           = 0xA5u;    
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = sizeof(write_byte);    
       
    const ssize_t write_ret = f_handle->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);    
}


static void sequence_generate(uint8_t* write_byte, uint8_t count)
{
    while (count != 0) {     
        *write_byte = count;
        
        ++write_byte;
        --count;
    }
}


/*
 * TC - MAX length UIH frame TX in 1 write call: upper bound test
 */
TEST(MultiplexerOpenTestGroup, user_tx_size_upper_bound)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1u;
    FileHandle* f_handle  = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);   
    f_handle->sigio(user_tx_size_lower_bound_tx_callback);
    
    /* Program write cycle. */
    uint8_t write_byte[TX_BUFFER_SIZE] = {0};
    write_byte[0]                      = FLAG_SEQUENCE_OCTET;
    write_byte[1]                      = 3u | (dlci_id << 2);
    write_byte[2]                      = FRAME_TYPE_UIH;
    write_byte[3]                      = LENGTH_INDICATOR_OCTET | ((TX_BUFFER_SIZE - 6u) << 1);
    
    sequence_generate(&(write_byte[4]), (sizeof(write_byte) - 6u));
    
    write_byte[TX_BUFFER_SIZE - 2]     = fcs_calculate(&write_byte[1], 3u);
    write_byte[TX_BUFFER_SIZE - 1]     = FLAG_SEQUENCE_OCTET;
    
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = mock_write->input_param[1].param;    
        
    const ssize_t ret = f_handle->write(&(write_byte[4]), (TX_BUFFER_SIZE - 6u));
    CHECK_EQUAL((TX_BUFFER_SIZE - 6u), ret);
}


static void user_tx_2_full_frame_writes_tx_callback()
{
    FAIL("TC FAILURE IF CALLED");
}


/*
 * TC - 2 sequential UIH frame TX in 1 write call
 */
TEST(MultiplexerOpenTestGroup, user_tx_2_full_frame_writes)
{
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1u;
    FileHandle* f_handle  = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);   
    f_handle->sigio(user_tx_2_full_frame_writes_tx_callback);
    
    /* Program write cycle, complete in 1 write call within the call context. */
    uint8_t user_data = 0xA5u;
    const uint8_t write_byte_1[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte_1[1], 3u),
        FLAG_SEQUENCE_OCTET
    };    
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte_1[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte_1);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = mock_write->input_param[1].param;

    ssize_t write_ret = f_handle->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);    
    
    /* Program write cycle, complete in 1 write call within the call context. */    
    ++user_data;
    const uint8_t write_byte_2[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte_2[1], 3u),
        FLAG_SEQUENCE_OCTET
    };                      
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte_2[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte_2);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = mock_write->input_param[1].param;        

    write_ret = f_handle->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);    
}


void single_complete_write_cycle(const uint8_t *write_byte, 
                                 uint8_t        length, 
                                 const uint8_t *new_write_byte)
{
    mock_t * mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value                             = 1;        
    const mbed::FileHandleMock::io_control_t io_control = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};
    mbed::FileHandleMock::io_control(io_control);    
    
    /* Nothing to read within the RX cycle. */
    mock_t * mock_read = mock_free_get("read");
    CHECK(mock_read != NULL);
    mock_read->output_param[0].param       = NULL;
    mock_read->output_param[0].len         = 0;    
    mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_read->input_param[0].param        = FRAME_HEADER_READ_LEN;    
    mock_read->return_value                = -EAGAIN;                         
    
    /* Complete the 1st write request which is in progress. */
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)(write_byte);        
    mock_write->input_param[1].param        = length;
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = mock_write->input_param[1].param;        
   
   if (new_write_byte != NULL) {
       /* Complete the write request of pending request frame. */        
       
        const uint8_t length_of_frame = 4u + (new_write_byte[3] & ~1) + 2u; // @todo: FIX ME: magic numbers. 
        
        mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)(new_write_byte);        
        mock_write->input_param[1].param        = length_of_frame;
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->return_value                = mock_write->input_param[1].param;                
        
        /* Request frame write sequence completed, start T1 timer. */   
            
        mock_t * mock_call_in = mock_free_get("call_in");    
        CHECK(mock_call_in != NULL);     
        mock_call_in->return_value = T1_TIMER_EVENT_ID;        
        mock_call_in->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_call_in->input_param[0].param        = T1_TIMER_VALUE;                          
   
   }
    
    /* Trigger deferred call to execute the programmed mocks above. */
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    mbed::EventQueueMock::io_control(eq_io_control);
}


static void user_tx_dlci_establish_during_user_tx_tx_callback()
{
    FAIL("TC FAILURE");
}


static void user_tx_dlci_establish_during_user_tx_sem_wait(const void *context)
{
    /* 3. Finish TX cycle for user TX, which triggers the TX callback above. */
    const uint8_t write_byte_sabm[6] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | ((DLCI_ID_LOWER_BOUND + 1u) << 2),         
        (FRAME_TYPE_SABM | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&write_byte_sabm[1], 3u),
        FLAG_SEQUENCE_OCTET
    };
    const uint8_t* write_byte_uih = (const uint8_t*)context;
    single_complete_write_cycle(&(write_byte_uih[0]), (UIH_FRAME_LEN - 1u), &(write_byte_sabm[0]));    
    
    /* 4. Finish the pending DLCI establishment cycle. */
    const uint8_t read_byte_sabm[5] = 
    {
        3u | ((DLCI_ID_LOWER_BOUND + 1u) << 2),         
        (FRAME_TYPE_UA | PF_BIT), 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte_sabm[0], 3u),
        FLAG_SEQUENCE_OCTET
    };
    self_iniated_response_rx(&(read_byte_sabm[0]), 
                             sizeof(read_byte_sabm), 
                             NULL,
                             SKIP_FLAG_SEQUENCE_OCTET,
                             STRIP_FLAG_FIELD_NO);            
}


/*
 * TC - Ensure successfull DLCI establishment is done when TX is occupied by user TX request
 *
 * Test sequence:
 * 1. Occupy TX by user TX
 * 2. Start DLCI establishment => put pending as TX occupied
 * 3. Finish the user TX
 * 4. FInish the DLCI establishment put pending in phase 2
 * 
 * Expected outcome:
 * - Requested DLCI established
 */
TEST(MultiplexerOpenTestGroup, user_tx_dlci_establish_during_user_tx)
{   
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
  
    FileHandle* f_handle = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);   
    f_handle->sigio(user_tx_dlci_establish_during_user_tx_tx_callback);
    
    /* 1. Start user TX write cycle, not finished. */
    const uint8_t user_data         = 0xA5u;    
    const uint8_t write_byte_uih[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (DLCI_ID_LOWER_BOUND << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte_uih[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte_uih[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte_uih);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte_uih[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte_uih) - sizeof(write_byte_uih[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;            

    const ssize_t write_ret  = f_handle->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);
    
    /* 2. Start new DLCI establishment while user TX in progress, put pending. */
    mock_t * mock_wait = mock_free_get("wait");
    CHECK(mock_wait != NULL);
    mock_wait->return_value = 1;
    mock_wait->func         = user_tx_dlci_establish_during_user_tx_sem_wait;   
    mock_wait->func_context = &(write_byte_uih[1]);    

    /* Start test sequence. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);  
    FileHandle *obj = NULL;
    const mbed::Mux::MuxReturnStatus establish_ret = mbed::Mux::dlci_establish((DLCI_ID_LOWER_BOUND + 1u), 
                                                                               status,
                                                                               &obj);
    CHECK_EQUAL(mbed::Mux::MUX_STATUS_SUCCESS, establish_ret);
    CHECK_EQUAL(mbed::Mux::MUX_ESTABLISH_SUCCESS, status);      
    CHECK(obj != NULL);
}


static uint8_t m_user_tx_callback_triggered_tx_within_callback_check_value = 0;
static void tx_callback_dispatch_triggered_tx_within_callback_tx_callback()
{
    static const uint8_t user_data = 2u;    
    /* Needs to be static as referenced after this function returns. */ 
    static const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (1u << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };                                   
    
    switch (m_user_tx_callback_triggered_tx_within_callback_check_value) {
        case 0:
            m_user_tx_callback_triggered_tx_within_callback_check_value = 1u;
            
            /* Issue new write to the same DLCI within the callback context. */
                                             
            mock_t * mock_write = mock_free_get("write");
            CHECK(mock_write != NULL); 
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
            mock_write->input_param[1].param        = sizeof(write_byte);
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            /* Write all in a 1 write request. */
            mock_write->return_value                = mock_write->input_param[1].param;    

            /* This write is started when this callback function returns. */
            ssize_t ret = m_file_handle[0]->write(&user_data, sizeof(user_data));
            CHECK_EQUAL(sizeof(user_data), ret);   

            /* This write request will set the pending TX callback, and triggers this function to be called 2nd time. */
            const uint8_t user_data_2 = 0xA5u;
            ret                       = m_file_handle[0]->write(&user_data_2, sizeof(user_data_2));
            CHECK_EQUAL(0, ret);  
            
            break;
        case 1:
            m_user_tx_callback_triggered_tx_within_callback_check_value = 2u;     
            
            break;
        default:
            FAIL("FAIL");
            
            break;
    }
}


/*
 * TC - 1 byte length UIH frame TX in 1 write call
 * - TX pending cllback called
 * - new TX done within the callback
 */
TEST(MultiplexerOpenTestGroup, tx_callback_dispatch_triggered_tx_within_callback)
{
    m_user_tx_callback_triggered_tx_within_callback_check_value = 0;

    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1u;
    m_file_handle[0]      = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);   
    (m_file_handle[0])->sigio(tx_callback_dispatch_triggered_tx_within_callback_tx_callback);
    
    /* Program write cycle. */
    const uint8_t user_data     = 1u;
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        
   
    /* 1st write request accepted by the implementation. */
    ssize_t ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), ret);
    
    /* 1st write request not yet completed by the implementation, issue 2nd request which sets the pending TX callback. 
    */
    const uint8_t user_data_2 = 0xA5u;
    ret                       = (m_file_handle[0])->write(&user_data_2, sizeof(user_data_2));
    CHECK_EQUAL(0, ret);
    
    /* Begin sequence: Complete the 1st write, which triggers the pending TX callback. */    

    single_complete_write_cycle(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), NULL);

    /* Validate proper callback sequence. */
    CHECK_EQUAL(2, m_user_tx_callback_triggered_tx_within_callback_check_value);
    
    /* End sequence: Complete the 1st write, which triggers the pending TX callback. */    
}

static uint8_t m_user_tx_callback_set_pending_multiple_times_for_same_dlci_only_1_callback_generated_value = 0;
static void tx_callback_dispatch_set_pending_multiple_times_for_same_dlci_only_1_callback_generated_cb()
{
    ++m_user_tx_callback_set_pending_multiple_times_for_same_dlci_only_1_callback_generated_value;
}


/*
 * TC - TX callback pending is set multiple times for same DLCI only 1 callback gets generated.
 */
TEST(MultiplexerOpenTestGroup, tx_callback_dispatch_set_pending_multiple_times_for_same_dlci_only_1_callback_generated)
{
    m_user_tx_callback_set_pending_multiple_times_for_same_dlci_only_1_callback_generated_value = 0;

    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    const uint8_t dlci_id = 1u;
    m_file_handle[0]      = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);   
    
(m_file_handle[0])->sigio(tx_callback_dispatch_set_pending_multiple_times_for_same_dlci_only_1_callback_generated_cb);
    
    /* Program write cycle. */
    const uint8_t user_data     = 1u;
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        
   
    /* 1st write request accepted by the implementation. */
    ssize_t ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), ret);
    
    /* 1st write request not yet completed by the implementation, issue 2 more requests which sets the same pending TX 
       callback. */
    uint8_t user_data_2 = 0xA5u;
    uint8_t i           = 2u;
    do {
        ret = (m_file_handle[0])->write(&user_data_2, sizeof(user_data_2));
        CHECK_EQUAL(0, ret); 
        
        ++user_data_2;
        --i;
    } while (i != 0);
    
    /* Begin sequence: Complete the 1st write, which triggers the pending TX callback. */    
    
    single_complete_write_cycle(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), NULL);

    /* Validate proper callback sequence. */
    CHECK_EQUAL(1u, m_user_tx_callback_set_pending_multiple_times_for_same_dlci_only_1_callback_generated_value);
    
    /* End sequence: Complete the 1st write, which triggers the pending TX callback. */        
}


static uint8_t m_user_tx_callback_set_pending_for_all_dlcis_check_value = 0;
static void tx_callback_dispatch_set_pending_for_all_dlcis_tx_callback()
{
    ++m_user_tx_callback_set_pending_for_all_dlcis_check_value;
}


/*
 * TC - all channels have TX callback pending 
 * - Expected bahaviour: Correct amount of callbacks executed
 */
TEST(MultiplexerOpenTestGroup, tx_callback_dispatch_set_pending_for_all_dlcis)
{
    m_user_tx_callback_set_pending_for_all_dlcis_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    /* Create max amount of DLCIs and collect the handles */
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    for (uint8_t i = 0; i!= MAX_DLCI_COUNT; ++i) {
        m_file_handle[i] = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);             
        CHECK(m_file_handle[i] != NULL);
        (m_file_handle[i])->sigio(tx_callback_dispatch_set_pending_for_all_dlcis_tx_callback);
        
        ++dlci_id;
    }    
    
    /* All available DLCI ids consumed. Next request will fail. */
    mbed::Mux::MuxEstablishStatus status(mbed::Mux::MUX_ESTABLISH_MAX);    
    FileHandle *obj              = NULL;
    const mbed::Mux::MuxReturnStatus establish_ret = mbed::Mux::dlci_establish(dlci_id, status, &obj);
    CHECK_EQUAL(establish_ret, mbed::Mux::MUX_STATUS_NO_RESOURCE);    
    CHECK_EQUAL(obj, NULL);  
   
    /* Program write cycle. */
    dlci_id                     = DLCI_ID_LOWER_BOUND;    
    const uint8_t user_data     = 1u;
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        
   
    /* 1st write request accepted by the implementation. */
    ssize_t write_ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);
    
    /* TX cycle in progress, all further write request will fail. */
    for (uint8_t i = 0; i!= MAX_DLCI_COUNT; ++i) {
        ssize_t write_ret = (m_file_handle[i])->write(&user_data, sizeof(user_data));
        CHECK_EQUAL(0, write_ret);        
    }
    
    /* Begin sequence: Complete the 1st write, which triggers the pending TX callback. */    
    
    single_complete_write_cycle(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), NULL);

    /* Validate proper callback sequence. */
    CHECK_EQUAL(MAX_DLCI_COUNT, m_user_tx_callback_set_pending_for_all_dlcis_check_value);
    
    /* End sequence: Complete the 1st write, which triggers the pending TX callback. */            
}


static uint8_t m_user_tx_callback_rollover_tx_pending_bitmask_check_value = 0;
static void tx_callback_dispatch_rollover_tx_pending_bitmask_tx_callback()
{
    ++m_user_tx_callback_rollover_tx_pending_bitmask_check_value;
    
    if (m_user_tx_callback_rollover_tx_pending_bitmask_check_value == MAX_DLCI_COUNT) {
        /* Callback for the last DLCI in the sequence, set pending bit for the 1st DLCI in the sequence. */

        static const uint8_t user_data = 2u;    
        /* Needs to be static as referenced after this function returns. */ 
        static const uint8_t write_byte[7] = 
        {
            FLAG_SEQUENCE_OCTET,
            3u | (DLCI_ID_LOWER_BOUND << 2),        
            FRAME_TYPE_UIH, 
            LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
            user_data,
            fcs_calculate(&write_byte[1], 3u),
            FLAG_SEQUENCE_OCTET
        };                                   
    
        mock_t * mock_write = mock_free_get("write");
        CHECK(mock_write != NULL); 
        mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
        mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
        mock_write->input_param[1].param        = sizeof(write_byte);
        mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
        /* Write all in a 1 write request, which will guarantee callback processing continues within current disptach 
           loop. */
        mock_write->return_value                = mock_write->input_param[1].param;    
            
        /* 1st write request accepted by the implementation: TX cycle not finished. */
        ssize_t write_ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
        CHECK_EQUAL(sizeof(user_data), write_ret);     
        
        /* TX cycle start requested by write call above, now set pending bit for the 1st DLCI of the sequence. */
        write_ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
        CHECK_EQUAL(0, write_ret);                
    }
}


/*
 * TC - Ensure proper roll over of the bitmask used for determining the disptaching of correct TX callback
 * Test sequence:
 * 1. Establish max amount of DLCIs
 * 2. Set TX pending bit for all establish DLCIs
 * 3. Within the TX callback of last DLCI of the sequence, set pending bit of the 1st DLCI of the sequence
 * 
 * Expected outcome:
 * - Validate proper TX callback callcount in m_user_tx_callback_rollover_tx_pending_bitmask_check_value
 */
TEST(MultiplexerOpenTestGroup, tx_callback_dispatch_rollover_tx_pending_bitmask)
{
    m_user_tx_callback_rollover_tx_pending_bitmask_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    /* Create max amount of DLCIs and collect the handles. */
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    for (uint8_t i = 0; i!= MAX_DLCI_COUNT; ++i) {
        m_file_handle[i] = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);             
        CHECK(m_file_handle[i] != NULL);
        (m_file_handle[i])->sigio(tx_callback_dispatch_rollover_tx_pending_bitmask_tx_callback);
        
        ++dlci_id;
    }    
    
    /* Start write cycle for the 1st DLCI. */
    dlci_id                     = DLCI_ID_LOWER_BOUND;    
    const uint8_t user_data     = 1u;
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        
   
    /* 1st write request accepted by the implementation: TX cycle not finished. */
    ssize_t write_ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);    
    
    /* TX cycle in progress, set TX pending bit for all established DLCIs. */
    for (uint8_t i = 0; i!= MAX_DLCI_COUNT; ++i) {
        write_ret = (m_file_handle[i])->write(&user_data, sizeof(user_data));
        CHECK_EQUAL(0, write_ret);        
    }    
    
    /* Begin sequence: Complete the 1st write, which triggers the pending TX callback. */    
    
    single_complete_write_cycle(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), NULL);

    /* Validate proper TX callback callcount. */
    CHECK_EQUAL((MAX_DLCI_COUNT +1u), m_user_tx_callback_rollover_tx_pending_bitmask_check_value);
    
    /* End sequence: Complete the 1st write, which triggers the pending TX callback. */                
}


static uint8_t m_user_tx_callback_tx_to_different_dlci_check_value = 0;
static void tx_callback_dispatch_tx_to_different_dlci_tx_callback()
{
    static const uint8_t user_data = 2u;    
    /* Needs to be static as referenced after this function returns. */ 
    static const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | ((DLCI_ID_LOWER_BOUND +1u) << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };
        
    switch (m_user_tx_callback_tx_to_different_dlci_check_value) {
        mock_t * mock_write;
        ssize_t write_ret;
        case 0:
            /* Current context is TX callback for the 1st handle. */
            
            ++m_user_tx_callback_tx_to_different_dlci_check_value;
            
            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL); 
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
            mock_write->input_param[1].param        = sizeof(write_byte);
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            /* Write all in a 1 write request, which will guarantee callback processing continues within current 
             * disptach loop. */
            mock_write->return_value = mock_write->input_param[1].param;    

            /* Start TX to 2nd handle. */
            write_ret = (m_file_handle[1])->write(&user_data, sizeof(user_data));
            CHECK_EQUAL(sizeof(user_data), write_ret);               
            break;
        case 1:
            /* Current context is TX callback for the 2nd handle. */
            
            ++m_user_tx_callback_tx_to_different_dlci_check_value;            
            break;
        default:
            /*No implementtaion required. Proper callback count enforced within the test body. */
            break;
    }
}


/*
 * TC - Ensure correct TX callbac count when doing TX, from TX callback, to a different DLCI than the current TX 
 * callback
 * 
 * @note: The current implementation is not optimal as If user is starting a TX to a DLCI, which is after the current 
 *        DLCI TX callback within the stored sequence this will result to dispatching 1 unnecessary TX callback, if this 
 *        is a issue one should clear the TX callback pending bit marker for this DLCI in @ref Mux::user_data_tx(...)
 *        in the place having @note and update this TC accordingly
 * 
 * Test sequence:
 * 1. Establish 2 DLCIs
 * 2. Set TX pending bit for all establish DLCIs
 * 3. Within 1st DLCI callback issue write for 2nd DLCI of the sequence, which completes the TX cycle within the call 
 *    context
 * 
 * Expected outcome:
 * - Validate proper TX callback callcount in m_user_tx_callback_tx_to_different_dlci_check_value
 */
TEST(MultiplexerOpenTestGroup, tx_callback_dispatch_tx_to_different_dlci_within_current_context)
{
    m_user_tx_callback_tx_to_different_dlci_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    /* Create 2 DLCIs and collect the handles. */
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    for (uint8_t i = 0; i!= 2u; ++i) {
        m_file_handle[i] = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);             
        CHECK(m_file_handle[i] != NULL);
        (m_file_handle[i])->sigio(tx_callback_dispatch_tx_to_different_dlci_tx_callback);
        
        ++dlci_id;
    }

    /* Start write cycle for the 1st DLCI. */
    dlci_id                     = DLCI_ID_LOWER_BOUND;    
    const uint8_t user_data     = 1u;
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        
   
    /* 1st write request accepted by the implementation: TX cycle not finished. */
    ssize_t write_ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);    
    
    /* TX cycle in progress, set TX pending bit for all established DLCIs. */
    for (uint8_t i = 0; i!= 2u; ++i) {
        write_ret = (m_file_handle[i])->write(&user_data, sizeof(user_data));
        CHECK_EQUAL(0, write_ret);        
    }    
    
    /* Begin sequence: Complete the 1st write, which triggers the pending TX callback. */    
    
    single_complete_write_cycle(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), NULL);
    
    /* Validate proper TX callback callcount. */
    CHECK_EQUAL(2u, m_user_tx_callback_tx_to_different_dlci_check_value);
    
    /* End sequence: Complete the 1st write, which triggers the pending TX callback. */                    
}

static uint8_t m_write_byte[7];

static uint8_t m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value = 0;
static void tx_callback_dispatch_tx_to_different_dlci_not_within_current_context_tx_callback()
{
    const uint8_t user_data = 2u;    
    
    /* Needs to be static as referenced after this function returns. */
    m_write_byte[0] = FLAG_SEQUENCE_OCTET;
    m_write_byte[1] = 3u | ((DLCI_ID_LOWER_BOUND +1u) << 2);        
    m_write_byte[2] = FRAME_TYPE_UIH;
    m_write_byte[3] = LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1);
    m_write_byte[4] = user_data;
    m_write_byte[5] = fcs_calculate(&m_write_byte[1], 3u);
    m_write_byte[6] = FLAG_SEQUENCE_OCTET;
        
    switch (m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value) {
        mock_t * mock_write;
        ssize_t write_ret;
        case 0:
            /* Current context is TX callback for the 1st handle. */
            
            ++m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value;

            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL); 
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(m_write_byte[0]);        
            mock_write->input_param[1].param        = sizeof(m_write_byte);
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 1;    
            
            mock_write = mock_free_get("write");
            CHECK(mock_write != NULL); 
            mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->input_param[0].param        = (uint32_t)&(m_write_byte[1]);        
            mock_write->input_param[1].param        = sizeof(m_write_byte) - sizeof(m_write_byte[0]);
            mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
            mock_write->return_value                = 0;                    

            /* Start TX to 2nd handle: TX cycle not finished within the current context. */
            write_ret = (m_file_handle[1])->write(&user_data, sizeof(user_data));
            CHECK_EQUAL(sizeof(user_data), write_ret);               
            break;
        case 1:
            /* Current context is TX callback for the 2nd handle. */
            
            ++m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value;            
            break;
        default:
            /*No implementation required. Proper callback count enforced within the test body. */
            break;
    }
}


/*
 * TC - Ensure correct TX callbac count when doing TX, from TX callback, to a different DLCI than the current TX 
 * callback
 * 
 * @note: The current implementation is not optimal as If user is starting a TX to a DLCI, which is after the current 
 *        DLCI TX callback within the stored sequence this will result to dispatching 1 unnecessary TX callback, if this
 *        is a issue one should clear the TX callback pending bit marker for this DLCI in @ref Mux::user_data_tx(...)
 *        in the place having @note and update this TC accordingly
 * 
 * Test sequence:
 * 1. Establish 2 DLCIs
 * 2. Set TX pending bit for all establish DLCIs
 * 3. Within 1st DLCI callback issue write for 2nd DLCI of the sequence, which does NOT complete the TX cycle within
 *    the call context
 * 
 * Expected outcome:
 * - Validate proper TX callback callcount in 
 *   m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value
 */
TEST(MultiplexerOpenTestGroup, tx_callback_dispatch_tx_to_different_dlci_not_within_current_context)
{
    m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
   
    /* Create 2 DLCIs and collect the handles. */
    uint8_t dlci_id = DLCI_ID_LOWER_BOUND;
    for (uint8_t i = 0; i!= 2u; ++i) {
        m_file_handle[i] = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);             
        CHECK(m_file_handle[i] != NULL);
        (m_file_handle[i])->sigio(tx_callback_dispatch_tx_to_different_dlci_not_within_current_context_tx_callback);
        
        ++dlci_id;
    }

    /* Start write cycle for the 1st DLCI. */
    dlci_id                     = DLCI_ID_LOWER_BOUND;    
    const uint8_t user_data     = 1u;
    const uint8_t write_byte[7] = 
    {
        FLAG_SEQUENCE_OCTET,
        3u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&write_byte[1], 3u),
        FLAG_SEQUENCE_OCTET
    };               
    mock_t * mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[0]);        
    mock_write->input_param[1].param        = sizeof(write_byte);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 1;    
    
    mock_write = mock_free_get("write");
    CHECK(mock_write != NULL); 
    mock_write->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->input_param[0].param        = (uint32_t)&(write_byte[1]);        
    mock_write->input_param[1].param        = sizeof(write_byte) - sizeof(write_byte[0]);
    mock_write->input_param[1].compare_type = MOCK_COMPARE_TYPE_VALUE;
    mock_write->return_value                = 0;        
   
    /* 1st write request accepted by the implementation: TX cycle not finished. */
    ssize_t write_ret = (m_file_handle[0])->write(&user_data, sizeof(user_data));
    CHECK_EQUAL(sizeof(user_data), write_ret);    
    
    /* TX cycle in progress, set TX pending bit for all established DLCIs. */
    for (uint8_t i = 0; i!= 2u; ++i) {
        write_ret = (m_file_handle[i])->write(&user_data, sizeof(user_data));
        CHECK_EQUAL(0, write_ret);        
    }    
    
    /* Complete the 1st write, which triggers the pending TX callback. */       
    single_complete_write_cycle(&(write_byte[1]), (sizeof(write_byte) - sizeof(write_byte[0])), NULL);
    
    /* TX started, but not finished, to 2nd DLCI within the 1st DLCI callback. Finish the TX cycle. */
    single_complete_write_cycle(&(m_write_byte[1]), (sizeof(m_write_byte) - sizeof(m_write_byte[0])), NULL);
    
    /* Validate proper TX callback callcount. */
    CHECK_EQUAL(2u, m_user_tx_callback_tx_to_different_dlci_not_within_current_context_check_value);
}


void single_complete_read_cycle(const uint8_t *read_byte, 
                                uint8_t        length)
{
    mock_t * mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value                             = 1;        
    const mbed::FileHandleMock::io_control_t io_control = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};
    mbed::FileHandleMock::io_control(io_control);
   
    /* Phase 1: read header length. */
    uint8_t rx_count = 0;
    mock_t * mock_read = mock_free_get("read");
    CHECK(mock_read != NULL); 
    mock_read->output_param[0].param       = &(read_byte[rx_count]);
    mock_read->input_param[0].param        = FRAME_HEADER_READ_LEN;
    mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;    
    mock_read->output_param[0].len         = mock_read->input_param[0].param;
    mock_read->return_value                = mock_read->output_param[0].len;
        
    /* Phase 2: read remainder of the frame. */
    rx_count += mock_read->return_value;
    mock_read = mock_free_get("read");
    CHECK(mock_read != NULL); 
    mock_read->output_param[0].param       = &(read_byte[rx_count]);
    mock_read->input_param[0].param        = (length - rx_count);   
    mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;    
    mock_read->output_param[0].len         = mock_read->input_param[0].param;
    mock_read->return_value                = mock_read->output_param[0].len;
    
    /* Verify internal logic. */
    rx_count += mock_read->return_value;
    CHECK_EQUAL(rx_count, length);

    /* Trigger deferred call to execute the programmed mocks above. */
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    mbed::EventQueueMock::io_control(eq_io_control);    
}


static void user_rx_0_length_user_payload_callback()
{
    FAIL("TC FAILURE IF CALLED");        
}


/*
 * TC - Ensure read failure for 0 user data length Rx cycle
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Generate user RX data with 0 length user data
 * 
 * Expected outcome:
 * - User read return -EAGAIN
 * - No callback called
 */
TEST(MultiplexerOpenTestGroup, user_rx_0_length_user_payload)
{    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(user_rx_0_length_user_payload_callback);    
    
    /* Start read cycle for the DLCI. */
    const uint8_t read_byte[5] = 
    {
        1u | (DLCI_ID_LOWER_BOUND << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };      
    
    single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));
    
    /* Verify read failure after successfull read cycle. */
    uint8_t buffer[1]      = {0};
    const ssize_t read_ret = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(-EAGAIN, read_ret);        
}


static uint8_t m_user_rx_single_read_check_value = 0;
static void user_rx_single_read_callback()
{
    ++m_user_rx_single_read_check_value;  
    
    mock_t * mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value = 1;            
    uint8_t buffer[1]       = {0};
    ssize_t read_ret        = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK(read_ret == sizeof(buffer));
    CHECK_EQUAL(0xA5u, buffer[0]);
    
    /* Verify failure after successfull read cycle. */
    read_ret = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(-EAGAIN, read_ret);    
}


/*
 * TC - Ensure the following for a single complete user data read cycle:
 * - correct RX callback count
 * - correct user payload content
 * - correct user payload length
 * - 2nd read request will return appropriate error to inform no data available for read
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Generate user RX data
 * 3. Issue 1st read in the callback
 * 4. Issue 2nd read in the callback
 * 
 * Expected outcome:
 * - as specified in TC description
 */
TEST(MultiplexerOpenTestGroup, user_rx_single_read)
{
    m_user_rx_single_read_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(user_rx_single_read_callback);
    
    /* Start read cycle for the DLCI. */
    const uint8_t user_data    = 0xA5u;
    const uint8_t read_byte[6] = 
    {
        1u | (DLCI_ID_LOWER_BOUND << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };      
    
    single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));
    
    /* Validate proper callback callcount. */
    CHECK_EQUAL(1, m_user_rx_single_read_check_value);
}


static void user_rx_single_read_no_data_available_callback()
{
    FAIL("TC FAILURE IF CALLED");
}


/*
 * TC - Ensure the following for a single complete user data read request:
 * - read request will return appropriate error to inform no data available for read
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Issue read request
 * 
 * Expected outcome:
 * - as specified in TC description
 */
TEST(MultiplexerOpenTestGroup, user_rx_single_read_no_data_available)
{    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(user_rx_single_read_no_data_available_callback);
    
    uint8_t buffer[1]      = {0};
    const ssize_t read_ret = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK(read_ret == -EAGAIN);
}


static uint8_t m_user_rx_rx_suspend_rx_resume_cycle_check_value = 0;
static void user_rx_rx_suspend_rx_resume_cycle_callback()
{
    ++m_user_rx_rx_suspend_rx_resume_cycle_check_value ;
}


/*
 * TC - Ensure the following: 
 * - Rx path procssing is suspended (no read cycle is started) upon reception of a valid user data frame.
 * - Rx path processing is resumed after valid user data frame has been consumed by the application 
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Generate user RX data frame
 * 3. Start Rx/Tx cycle
 * 5. Verify read buffer
 * 
 * Expected outcome:
 * - Rx path processing suspended (no read cycle is started) upon reception of a valid user data frame.
 * - Rx path processing enabled after valid user data frame has been consumed by the application 
 * - Correct callback count
 * - Read buffer verified
 */
TEST(MultiplexerOpenTestGroup, user_rx_rx_suspend_rx_resume_cycle)
{
    m_user_rx_rx_suspend_rx_resume_cycle_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();
    
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(user_rx_rx_suspend_rx_resume_cycle_callback);
    
    /* Start 1st read cycle for the DLCI. */
    uint8_t user_data    = 0xA5u;
    uint8_t read_byte[6] = 
    {
        1u | (DLCI_ID_LOWER_BOUND << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };         
    single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));

    /* Validate proper callback callcount. */
    CHECK_EQUAL(1, m_user_rx_rx_suspend_rx_resume_cycle_check_value );
    
    /* Start 2nd Rx/Tx cycle, which omits the Rx part as Rx prcossing has been suspended by reception of valid user 
       data frame above. Tx is omitted as there is no data in the Tx buffer. */
    mock_t * mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value                             = 1;        
    const mbed::FileHandleMock::io_control_t io_control = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};
    mbed::FileHandleMock::io_control(io_control);    
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE};
    mbed::EventQueueMock::io_control(eq_io_control);        
    
    /* Verify read buffer: consumption of the read buffer triggers enqueue to event Q. */
    mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value = 1;      
    uint8_t buffer[1]       = {0};
    ssize_t read_ret        = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(sizeof(buffer), read_ret);
    CHECK(buffer[0] == user_data);    
    read_ret = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(-EAGAIN, read_ret);    
       
    /* Verify that Rx processing has been resumed after read buffer has been consumed above. */
    
    /* Start 2nd read cycle for the DLCI. */
    user_data    = 0x5Au;
    read_byte[3] = user_data;
    read_byte[4] = fcs_calculate(&read_byte[0], 3u);
    
    single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));   
    
    /* Validate proper callback callcount. */
    CHECK_EQUAL(2, m_user_rx_rx_suspend_rx_resume_cycle_check_value );    
    
    /* Start 2nd Rx/Tx cycle, which omits the Rx part as Rx processing has been suspended by reception of valid user 
       data frame above. Tx is omitted as there is no data in the Tx buffer. */
    mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value = 1;       
    mbed::FileHandleMock::io_control(io_control);   
    mbed::EventQueueMock::io_control(eq_io_control);        
    
    /* Verify read buffer: consumption of the read buffer triggers enqueue to event Q. */
    mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value = 1;          
    buffer[0]               = 0;
    read_ret                = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(sizeof(buffer), read_ret);
    CHECK(buffer[0] == user_data);
    read_ret = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(-EAGAIN, read_ret);
}


void single_byte_read_cycle(const uint8_t *read_byte, 
                            uint8_t        length)
{
    CHECK(length >= 6); //@todo: MAGIC: 6 min size for UIH as payload min size is 1     
    
    const mbed::FileHandleMock::io_control_t io_control    = {mbed::FileHandleMock::IO_TYPE_SIGNAL_GENERATE};    
    const mbed::EventQueueMock::io_control_t eq_io_control = {mbed::EventQueueMock::IO_TYPE_DEFERRED_CALL_GENERATE}; 
  
    mock_t * mock_call;
    mock_t * mock_read;
    uint8_t current_read_len;      
    uint8_t rx_count = 0;    

    /* Phase 1: read header length. */    
    do {        
        mock_call = mock_free_get("call");
        CHECK(mock_call != NULL);           
        mock_call->return_value = 1;          
        /* Trigger sigio from underlying FileHandle to run programmed mock above. */
        mbed::FileHandleMock::io_control(io_control);
        
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(read_byte[rx_count]);
        mock_read->input_param[0].param        = (FRAME_HEADER_READ_LEN - rx_count);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;    
        mock_read->output_param[0].len         = 1u;
        mock_read->return_value                = mock_read->output_param[0].len;        
        
        if ((rx_count + 1u) != FRAME_HEADER_READ_LEN) {
            current_read_len = (FRAME_HEADER_READ_LEN - (rx_count + 1u));
        } else {
            /* We have entered phase 2 of the read cycle. */
            current_read_len = (length - (rx_count + 1u));
        }
            
        /* Stop the read cycle. */ 
        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = NULL;
        mock_read->input_param[0].param        = current_read_len;
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;    
        mock_read->output_param[0].len         = 0;
        mock_read->return_value                = -EAGAIN;
        
        /* Trigger deferred call to execute the programmed mocks above. */
        mbed::EventQueueMock::io_control(eq_io_control);            
        
        ++rx_count;
    } while (rx_count != FRAME_HEADER_READ_LEN);
    
    /* Phase 2: read remainder of the frame. */
    do {
        mock_call = mock_free_get("call");
        CHECK(mock_call != NULL);           
        mock_call->return_value = 1;          
        /* Trigger sigio from underlying FileHandle to run programmed mock above. */
        mbed::FileHandleMock::io_control(io_control);

        mock_read = mock_free_get("read");
        CHECK(mock_read != NULL); 
        mock_read->output_param[0].param       = &(read_byte[rx_count]);
        mock_read->input_param[0].param        = (length - rx_count);
        mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;    
        mock_read->output_param[0].len         = 1u;
        mock_read->return_value                = mock_read->output_param[0].len;                

        if ((rx_count + 1u) != length) {
            /* Stop the read cycle. */ 
            mock_read = mock_free_get("read");
            CHECK(mock_read != NULL); 
            mock_read->output_param[0].param       = NULL;
            mock_read->input_param[0].param        = (length - (rx_count + 1u));
            mock_read->input_param[0].compare_type = MOCK_COMPARE_TYPE_VALUE;    
            mock_read->output_param[0].len         = 0;
            mock_read->return_value                = -EAGAIN;            
        }
            
        /* Trigger deferred call to execute the programmed mocks above. */
        mbed::EventQueueMock::io_control(eq_io_control);
       
        ++rx_count;
    } while (rx_count != length);
}


static uint8_t m_user_rx_read_1_byte_per_run_context_check_value = 0;
static void user_rx_read_1_byte_per_run_context_callback()
{
    ++m_user_rx_read_1_byte_per_run_context_check_value;   
}


/*
 * TC - Ensure that Rx frame read works correctly when only 1 byte can be read from lower layer within run context.
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Generate user RX data frame
 * 3. Generate read cycles which only supply 1 byte at a time from lower layer
 * 4. Verify read buffer upon frame read complete
 * 
 * Expected outcome:
 * - Read buffer verified
 * - Correct callback count
 */
TEST(MultiplexerOpenTestGroup, user_rx_read_1_byte_per_run_context)
{
    m_user_rx_read_1_byte_per_run_context_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    mux_self_iniated_open();
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(user_rx_read_1_byte_per_run_context_callback);
    
    /* Start read cycle for the DLCI. */
    const uint8_t user_data    = 0xA5u;
    const uint8_t read_byte[6] = 
    {
        1u | (DLCI_ID_LOWER_BOUND << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };
    single_byte_read_cycle(&(read_byte[0]), sizeof(read_byte));
    
    /* Verify read buffer. */
    mock_t * mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value = 1;            
    uint8_t buffer[1]       = {0};
    const ssize_t read_ret  = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(sizeof(buffer), read_ret);
    CHECK_EQUAL(user_data, buffer[0]);        
    
    /* Validate proper callback callcount. */
    CHECK_EQUAL(1, m_user_rx_read_1_byte_per_run_context_check_value);
}


static uint8_t m_user_rx_read_max_size_user_payload_in_1_read_call_check_value = 0;
static void user_rx_read_max_size_user_payload_in_1_read_call_callback()
{
    ++m_user_rx_read_max_size_user_payload_in_1_read_call_check_value;
}


/*
 * TC - Ensure that Rx frame read works correctly when max amount of user data is received.
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Generate user RX data frame
 * 4. Verify read buffer upon frame read complete
 * 
 * Expected outcome:
 * - Read buffer verified
 * - Correct callback count
 */
TEST(MultiplexerOpenTestGroup, user_rx_read_max_size_user_payload_in_1_read_call)
{
    m_user_rx_read_max_size_user_payload_in_1_read_call_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    mux_self_iniated_open();
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(user_rx_read_max_size_user_payload_in_1_read_call_callback);
    
    /* Program read cycle. */
    uint8_t read_byte[RX_BUFFER_SIZE - 1u] = {0};
    read_byte[0]                           = 1u | (DLCI_ID_LOWER_BOUND << 2);
    read_byte[1]                           = FRAME_TYPE_UIH;
    read_byte[2]                           = LENGTH_INDICATOR_OCTET | ((sizeof(read_byte) - 5u) << 1);
    
    sequence_generate(&(read_byte[3]), (sizeof(read_byte) - 5u));
    
    read_byte[sizeof(read_byte) - 2] = fcs_calculate(&read_byte[0], 3u);
    read_byte[sizeof(read_byte) - 1] = FLAG_SEQUENCE_OCTET;    
    
    single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));    
    
    /* Verify read buffer. */
    mock_t * mock_call = mock_free_get("call");
    CHECK(mock_call != NULL);           
    mock_call->return_value             = 1;            
    uint8_t buffer[RX_BUFFER_SIZE - 6u] = {0};
    CHECK_EQUAL(sizeof(buffer), (sizeof(read_byte) - 5u));               
    const ssize_t read_ret = m_file_handle[0]->read(&(buffer[0]), sizeof(buffer));
    CHECK_EQUAL(sizeof(buffer), read_ret);    
    const int buffer_compare = memcmp(&(buffer[0]), &(read_byte[3]), sizeof(buffer));
    CHECK_EQUAL(0, buffer_compare);            
    
    /* Validate proper callback callcount. */
    CHECK_EQUAL(1, m_user_rx_read_max_size_user_payload_in_1_read_call_check_value);
}


static uint8_t m_user_rx_read_1_byte_per_read_call_max_size_user_payload_available_check_value = 0;
static void rx_read_1_byte_per_read_call_max_size_user_payload_available_callback()
{
    ++m_user_rx_read_1_byte_per_read_call_max_size_user_payload_available_check_value;
}


/*
 * TC - Ensure that Rx frame read works correctly when max amount of user data is received and read is done 1 byte a 
 * time.
 * 
 * Test sequence:
 * 1. Establish 1 DLCI
 * 2. Generate user RX data frame
 * 3. Verify read buffer upon frame read complete
 * 
 * Expected outcome:
 * - Read buffer verified
 * - Correct callback count
 */
TEST(MultiplexerOpenTestGroup, user_rx_read_1_byte_per_read_call_max_size_user_payload_available)
{
    m_user_rx_read_1_byte_per_read_call_max_size_user_payload_available_check_value = 0;
    
    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    mux_self_iniated_open();
    m_file_handle[0] = dlci_self_iniated_establish(ROLE_INITIATOR, DLCI_ID_LOWER_BOUND);    
    CHECK(m_file_handle[0] != NULL);
    m_file_handle[0]->sigio(rx_read_1_byte_per_read_call_max_size_user_payload_available_callback);
    
    /* Program read cycle. */
    uint8_t read_byte[RX_BUFFER_SIZE - 1u] = {0};
    read_byte[0]                           = 1u | (DLCI_ID_LOWER_BOUND << 2);
    read_byte[1]                           = FRAME_TYPE_UIH;
    read_byte[2]                           = LENGTH_INDICATOR_OCTET | ((sizeof(read_byte) - 5u) << 1);
    
    sequence_generate(&(read_byte[3]), (sizeof(read_byte) - 5u));
    
    read_byte[sizeof(read_byte) - 2] = fcs_calculate(&read_byte[0], 3u);
    read_byte[sizeof(read_byte) - 1] = FLAG_SEQUENCE_OCTET;    
    
    single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));    
    
    /* Verify read buffer: do reads 1 byte a time until all of the data is read. */
    mock_t * mock_call;
    ssize_t  read_ret;
    uint8_t  test_buffer[sizeof(read_byte) - 5u] = {0};    
    uint8_t  read_count = 0;
    do {
        if ((read_count + 1u) == sizeof(test_buffer)) {
            mock_call = mock_free_get("call");
            CHECK(mock_call != NULL);           
            mock_call->return_value = 1;                        
        }
        
        read_ret = m_file_handle[0]->read(&(test_buffer[read_count]), 1u);
        CHECK_EQUAL(1, read_ret);
        CHECK_EQUAL(read_byte[3u + read_count], test_buffer[read_count]);
        
        ++read_count;
    } while (read_count != sizeof(test_buffer));
    
    /* Verify read buffer empty. */
    read_ret = m_file_handle[0]->read(&(test_buffer[0]), 1u);
    CHECK_EQUAL(-EAGAIN, read_ret);
    
    /* Validate proper callback callcount. */
    CHECK_EQUAL(1, m_user_rx_read_1_byte_per_read_call_max_size_user_payload_available_check_value);    
}


static uint8_t m_user_rx_dlci_not_established_check_value = 0;
static void user_rx_dlci_not_established_callback()
{
    ++m_user_rx_dlci_not_established_check_value;
}


/*
 * TC - Ensure proper behaviour when user data Rx frame received to DLCI ID, which is not established.
 * 
 * Test sequence:
 * 1. Mux open
 * 2. Iterate through max amount of supported DLCI IDs following sequence:
 * - start read cycle for the not established DLCI 
 * - start read cycle for the established DLCI
 * 
 * Expected outcome:
 * - The Rx frame is dropped by the implementation for the not established DLCI
 * - The Rx frame is accepted by the implementation for the not established DLCI 
 * - Validate proper callback callcount
 * - Validate read buffer
 */
TEST(MultiplexerOpenTestGroup, user_rx_dlci_not_established)
{    
    m_user_rx_dlci_not_established_check_value = 0;

    mbed::FileHandleMock fh_mock;   
    mbed::EventQueueMock eq_mock;
    
    mbed::Mux::eventqueue_attach(&eq_mock);
       
    /* Set and test mock. */
    mock_t * mock_sigio = mock_free_get("sigio");    
    CHECK(mock_sigio != NULL);      
    mbed::Mux::serial_attach(&fh_mock);
    
    mux_self_iniated_open();

    uint8_t dlci_id      = DLCI_ID_LOWER_BOUND;    
    uint8_t user_data    = 0xA5u;
    uint8_t read_byte[6] = 
    {
        1u | (dlci_id << 2),        
        FRAME_TYPE_UIH, 
        LENGTH_INDICATOR_OCTET | (sizeof(user_data) << 1),
        user_data,
        fcs_calculate(&read_byte[0], 3u),
        FLAG_SEQUENCE_OCTET
    };         
    
    mock_t * mock_call;
    ssize_t  read_ret;
    uint8_t  test_buffer[1] = {0};    
    for (uint8_t i = 0; i!= MAX_DLCI_COUNT; ++i) {
        
        /* Start read cycle for the not established DLCI. */
        single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));
        
        /* Establish the DLCI. */
        m_file_handle[i] = dlci_self_iniated_establish(ROLE_INITIATOR, dlci_id);             
        CHECK(m_file_handle[i] != NULL);
        m_file_handle[i]->sigio(user_rx_dlci_not_established_callback);
        
        /* Validate proper callback callcount. */
        CHECK_EQUAL(i, m_user_rx_dlci_not_established_check_value);                
        
        /* Start read cycle for the established DLCI. */
        single_complete_read_cycle(&(read_byte[0]), sizeof(read_byte));

        /* Validate proper callback callcount. */
        CHECK_EQUAL((i + 1), m_user_rx_dlci_not_established_check_value);                
        
        /* Validate read buffer. */
        mock_call = mock_free_get("call");
        CHECK(mock_call != NULL);           
        mock_call->return_value = 1;                    
        read_ret = m_file_handle[i]->read(&(test_buffer[0]), 1u);
        CHECK_EQUAL(1, read_ret);
        CHECK_EQUAL(user_data, test_buffer[0]);        
        read_ret = m_file_handle[i]->read(&(test_buffer[0]), 1u);
        CHECK_EQUAL(-EAGAIN, read_ret);
        
        /* Construct new buffer for the next iteration. */       
        read_byte[0] = 1u | (++dlci_id << 2);
        read_byte[3] = ++user_data;
        read_byte[4] = fcs_calculate(&read_byte[0], 3u);
    }
}


} // namespace mbed
