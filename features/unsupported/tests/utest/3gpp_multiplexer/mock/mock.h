
#ifndef MBED_MOCK_H
#define MBED_MOCK_H

#include <stdint.h>
#include <stdbool.h>
    
typedef void (*callback_handler)(const void * context);

#define NAME_LEN           31u
#define INPUT_PARAM_COUNT  4u
#define OUTPUT_PARAM_COUNT INPUT_PARAM_COUNT

typedef enum
{
    MOCK_COMPARE_TYPE_NONE = 0, /* Default out-of-the-box type set by the mock implementation. */  
    MOCK_COMPARE_TYPE_NULL, 
    MOCK_COMPARE_TYPE_VALUE,    
    MOCK_COMPARE_TYPE_MAX,
} MockCompareType;

typedef struct
{
    uint32_t        param;
    MockCompareType compare_type;
} input_param_t;

typedef struct
{
    const void *param; /* Default out-of-the-box value is NULL, set by the mock implementation. */  
    uint32_t    len;
} output_param_t; 

typedef struct 
{
    uint32_t         return_value;
    callback_handler func;                              /* Default out-of-the-box value is NULL, set by the mock 
                                                           implementation. */  
    const void *     func_context;
    input_param_t    input_param[INPUT_PARAM_COUNT];
    output_param_t   output_param[OUTPUT_PARAM_COUNT];    
    uint8_t          is_valid : 1;    
    char             name[NAME_LEN];
} mock_t; 

/* Below ones to be called only outside the mock object. */
void mock_init(void);
void mock_verify(void);
mock_t * mock_free_get(const char *name);

/* Below ones to be called only within the mock object. */

typedef enum 
{
    MockInvalidateTypeYes = 0,
    MockInvalidateTypeNo,
    MockInvalidateTypeMax
} MockInvalidateType;        
mock_t * mock_open(const char *name, MockInvalidateType type);
void mock_invalidate(const char *name);

#endif
