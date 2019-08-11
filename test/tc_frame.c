#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_frame.h"
#include "lora_aes.h"
#include "lora_cmac.h"

#include <string.h>

static void encode_an_empty_frame(void **user)
{
    uint32_t devAddr = 0U;
    const uint8_t expected[] = "\x40\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00";
    const uint8_t dummyKey[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    struct lora_frame_data input;
    (void)memset(&input, 0, sizeof(input));
    uint8_t outLen;
    uint8_t out[UINT8_MAX];
    enum lora_frame_type type;

    type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    input.counter = 256;
    input.devAddr = devAddr;
     
    outLen = LDL_Frame_putData(type, dummyKey, dummyKey, &input, out, sizeof(out));
    
    assert_int_equal(sizeof(expected)-1U, outLen);
    assert_memory_equal(expected, out, sizeof(expected)-1U);
}

static void decode_an_empty_frame(void **user)
{
    uint32_t devAddr = 0U;
    uint8_t input[] = "\x40\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00";
    const uint8_t dummyKey[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    struct lora_frame expected;
    struct lora_frame output;
    bool result;
    enum lora_frame_type type;

    (void)memset(&expected, 0, sizeof(expected));
    
    type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    
    expected.type = type;
    expected.fields.data.counter = 256;
    expected.fields.data.devAddr = devAddr;

    result = LDL_Frame_decode(dummyKey, dummyKey, dummyKey, input, sizeof(input)-1U, &output);

    assert_true(result);
    
    assert_int_equal(expected.fields.data.counter, output.fields.data.counter);
    assert_int_equal(expected.fields.data.devAddr, output.fields.data.devAddr);
    assert_int_equal(expected.fields.data.ack, output.fields.data.ack);
    assert_int_equal(expected.fields.data.adr, output.fields.data.adr);
    assert_int_equal(expected.fields.data.adrAckReq, output.fields.data.adrAckReq);
    assert_int_equal(expected.fields.data.pending, output.fields.data.pending);
    
    assert_int_equal(expected.fields.data.optsLen, output.fields.data.optsLen);
    
    if(expected.fields.data.opts != NULL){
    
        assert_memory_equal(expected.fields.data.opts, output.fields.data.opts, sizeof(expected.fields.data.optsLen));
    }
    
    assert_int_equal(expected.fields.data.dataLen, output.fields.data.dataLen);
    
    if(expected.fields.data.data != NULL){
        
        assert_memory_equal(expected.fields.data.data, output.fields.data.opts, sizeof(expected.fields.data.optsLen));
    }
    
    //assert_int_equal(expected., output.);
    //ssert_int_equal(expected., output.);
    
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(encode_an_empty_frame),        
        cmocka_unit_test(decode_an_empty_frame),        
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}


