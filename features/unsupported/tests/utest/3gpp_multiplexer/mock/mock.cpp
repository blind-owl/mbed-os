#include "mock.h"
#include "TestHarness.h"
#include <stddef.h>
#include <string.h>

#define DB_LEN 4u

static mock_t db[DB_LEN];

void mock_init(void)
{
    for (uint8_t i = 0; i < (sizeof(db) / sizeof(db[0])); ++i) {
        db[i].is_valid  = 0;
    }
}


void mock_verify(void)
{
    for (uint8_t i = 0; i < (sizeof(db) / sizeof(db[0])); ++i) {
        if ((db[i].is_valid)) {
            UT_PRINT(db[i].name);
            FAIL("FAILURE: mock_verify!");
            UT_PRINT(db[i].name);
//UT_PRINT_LOCATION(db[i].name, "TRACE: ", 0);            
        }
    }
}


mock_t * mock_open(const char *name)
{
    mock_t * obj = NULL;
    
    /* Look-up database for correct mock. */
    for (uint8_t i = 0; i < (sizeof(db) / sizeof(db[0])); ++i) {
        if ((strcmp(name, db[i].name) == 0) && (db[i].is_valid)) {
            db[i].is_valid = 0;
            obj            = &db[i];
            
            break;
        }
    }
   
    return obj;
}

mock_t * mock_free_get(const char *name)
{
    mock_t * obj = NULL;
    
    /* Look-up database for free place and occupy it if found. */
    for (uint8_t i = 0; i < (sizeof(db) / sizeof(db[0])); ++i) {
        if (!(db[i].is_valid)) {
            strcpy(db[i].name, name);
            db[i].is_valid = 1;
            db[i].func     = NULL;
            for (uint8_t j = 0; j < (sizeof(db[0].input_param) / sizeof(db[0].input_param[0])); ++j) {           
                db[i].input_param[j].compare_type = MOCK_COMPARE_TYPE_NONE;                
            }
            for (uint8_t j = 0; j < (sizeof(db[0].output_param) / sizeof(db[0].output_param[0])); ++j) {       
                db[i].output_param[j].param = NULL;
            }

            obj = &db[i];                        
            
            break;
        }        
    }
    
    return obj;
}
