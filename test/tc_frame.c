#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "ldl_frame.h"

#include <string.h>

static void check_data_down(bool result, const struct ldl_frame_down *expected, const struct ldl_frame_down *output)
{
    assert_true(result);
    
    assert_int_equal(expected->counter, output->counter);
    assert_int_equal(expected->devAddr, output->devAddr);
    assert_int_equal(expected->ack, output->ack);
    assert_int_equal(expected->adr, output->adr);
    assert_int_equal(expected->adrAckReq, output->adrAckReq);
    assert_int_equal(expected->pending, output->pending);
    assert_int_equal(expected->mic, output->mic);
    
    assert_int_equal(expected->opts, output->opts);
    assert_int_equal(expected->optsLen, output->optsLen);

    if(expected->opts != NULL){
    
        assert_memory_equal(expected->opts, output->opts, sizeof(expected->optsLen));
    }
    
    assert_int_equal(expected->port, output->port);
    
    assert_int_equal(expected->data, output->data);
    assert_int_equal(expected->dataLen, output->dataLen);
    
    if(expected->data != NULL){
        
        assert_memory_equal(expected->data, output->data, sizeof(expected->dataLen));
    }
}

/* expectations *******************************************************/

static void encode_unconfirmed_data_up(void **user)
{
    const uint8_t expected[] = "\x40\x33\x22\x11\x00\x00\x00\x01\x77\x66\x55\x44";
    struct ldl_frame_data input;
    (void)memset(&input, 0, sizeof(input));
    uint8_t outLen;
    uint8_t out[UINT8_MAX];
    struct ldl_frame_data_offset off;

    input.type = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    input.counter = 256;
    input.devAddr = 0x00112233UL;
    input.mic = 0x44556677UL;
     
    outLen = LDL_Frame_putData(&input, out, sizeof(out), &off);
    
    assert_int_equal(sizeof(expected)-1U, outLen);
    assert_memory_equal(expected, out, sizeof(expected)-1U);
}

static void decode_shall_accept_empty_unconfirmed_data_down(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x00\x00\x01\x77\x66\x55\x44";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
    expected.counter = 256;
    expected.devAddr = 0x00112233UL;
    expected.mic = 0x44556677UL;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    check_data_down(result, &expected, &output);
}

static void decode_shall_accept_empty_unconfirmed_data_down_with_fopts(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x03\x00\x01\xaa\xaa\xaa\x77\x66\x55\x44";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
    expected.counter = 256;
    expected.devAddr = 0x00112233UL;
    expected.mic = 0x44556677UL;
    expected.opts = &input[8U];
    expected.optsLen = 3U;

    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    check_data_down(result, &expected, &output);
}

static void decode_shall_accept_unconfirmed_data_down(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x00\x00\x01\x01\xaa\xaa\xaa\x77\x66\x55\x44";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
    expected.counter = 256;
    expected.devAddr = 0x00112233UL;
    expected.mic = 0x44556677UL;
    expected.data = &input[9U]; // just after the port number
    expected.dataLen = 3U;
    expected.port = 1U;
    expected.dataPresent = true;

    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    check_data_down(result, &expected, &output);
}

static void decode_shall_accept_unconfirmed_data_down_with_fopts(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x02\x00\x01\xbb\xbb\x01\xaa\xaa\xaa\x77\x66\x55\x44";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
    expected.counter = 256;
    expected.devAddr = 0x00112233UL;
    expected.mic = 0x44556677UL;
    expected.opts = &input[8U];
    expected.optsLen = 2U;
    expected.data = &input[11U];
    expected.dataLen = 3U;
    expected.port = 1U;
    expected.dataPresent = true;

    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    check_data_down(result, &expected, &output);
}

static void decode_shall_accept_unconfirmed_data_down_with_port_and_nodata(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x00\x00\x01\x01\x77\x66\x55\x44";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_DATA_UNCONFIRMED_DOWN;
    expected.counter = 256;
    expected.devAddr = 0x00112233UL;
    expected.mic = 0x44556677UL;
    expected.port = 1U;
    expected.dataPresent = true;

    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    check_data_down(result, &expected, &output);
}

static void decode_shall_reject_unconfirmed_data_down_with_opts_and_port_zero(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x01\x00\x01\xaa\x00\x00\x77\x66\x55\x44";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_unconfirmed_data_down_with_opts_and_port_zero_and_nodata(void **user)
{
    uint8_t input[] = "\x60\x33\x22\x11\x00\x01\x00\x01\xaa\x00\x77\x66\x55\x44";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_accept_join_accept_without_cflist(void **user)
{
    uint8_t input[] = "\x20\x22\x11\x00\x55\x44\x33\x99\x88\x77\x66\x00\x00\xdd\xcc\xbb\xaa";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_JOIN_ACCEPT;
    expected.joinNonce = 0x001122U;
    expected.netID = 0x334455UL;
    expected.devAddr = 0x66778899UL;
    expected.mic = 0xaabbccddUL;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_true(result);
    
    assert_int_equal(expected.type, output.type);
    assert_int_equal(expected.joinNonce, output.joinNonce);
    assert_int_equal(expected.netID, output.netID);
    assert_int_equal(expected.devAddr, output.devAddr);
    assert_int_equal(expected.mic, output.mic);    
    
    assert_int_equal(expected.cfList, output.cfList);    
    assert_int_equal(expected.cfListLen, output.cfListLen);    
}



static void decode_shall_accept_join_accept_with_cflist(void **user)
{
    uint8_t input[] = "\x20\x22\x11\x00\x55\x44\x33\x99\x88\x77\x66\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\xdd\xcc\xbb\xaa";
    struct ldl_frame_down expected;
    struct ldl_frame_down output;
    bool result;
    
    (void)memset(&expected, 0, sizeof(expected));
    
    expected.type = FRAME_TYPE_JOIN_ACCEPT;
    expected.joinNonce = 0x001122U;
    expected.netID = 0x334455UL;
    expected.devAddr = 0x66778899UL;
    expected.mic = 0xaabbccddUL;
    expected.cfList = &input[17U-4U];
    expected.cfListLen = 16U;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_true(result);
    
    assert_int_equal(expected.type, output.type);
    assert_int_equal(expected.joinNonce, output.joinNonce);
    assert_int_equal(expected.netID, output.netID);
    assert_int_equal(expected.devAddr, output.devAddr);
    assert_int_equal(expected.mic, output.mic);    
    
    assert_int_equal(expected.cfList, output.cfList);    
    assert_int_equal(expected.cfListLen, output.cfListLen);    
}

static void decode_shall_reject_short_join_accept(void **user)
{
    uint8_t input[] = "\x20\x22\x11\x00\x55\x44\x33\x99\x88\x77\x66\x00\x00\xdd\xcc\xbb";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_short_join_accept_with_cflist(void **user)
{
    uint8_t input[] = "\x20\x22\x11\x00\x55\x44\x33\x99\x88\x77\x66\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\xdd\xcc\xbb";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_long_join_accept(void **user)
{
    uint8_t input[] = "\x20\x22\x11\x00\x55\x44\x33\x99\x88\x77\x66\x00\x00\xdd\xcc\xbb\xaa\x00";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_long_join_accept_with_cflist(void **user)
{
    uint8_t input[] = "\x20\x22\x11\x00\x55\x44\x33\x99\x88\x77\x66\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\xdd\xcc\xbb\xaa\x00";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_unconfirmed_data_up(void **user)
{
    uint8_t input[] = "\x40\x33\x22\x11\x00\x00\x00\x01\x77\x66\x55\x44";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_confirmed_data_up(void **user)
{
    uint8_t input[] = "\x80\x33\x22\x11\x00\x00\x00\x01\x77\x66\x55\x44";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_join_request(void **user)
{
    uint8_t input[] = "\x00";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

static void decode_shall_reject_rejoin_request(void **user)
{
    uint8_t input[] = "\x60";
    struct ldl_frame_down output;
    bool result;
    
    result = LDL_Frame_decode(&output, input, sizeof(input)-1U);

    assert_false(result);
}

/* runner *******************************************************/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(encode_unconfirmed_data_up),        
        cmocka_unit_test(decode_shall_accept_empty_unconfirmed_data_down),        
        cmocka_unit_test(decode_shall_accept_unconfirmed_data_down),        
        cmocka_unit_test(decode_shall_accept_empty_unconfirmed_data_down_with_fopts),        
        cmocka_unit_test(decode_shall_accept_unconfirmed_data_down_with_fopts),        
        cmocka_unit_test(decode_shall_accept_unconfirmed_data_down_with_port_and_nodata),        
        cmocka_unit_test(decode_shall_reject_unconfirmed_data_down_with_opts_and_port_zero),        
        cmocka_unit_test(decode_shall_reject_unconfirmed_data_down_with_opts_and_port_zero_and_nodata),        
        cmocka_unit_test(decode_shall_accept_join_accept_without_cflist),        
        cmocka_unit_test(decode_shall_accept_join_accept_with_cflist),        
        cmocka_unit_test(decode_shall_reject_short_join_accept),        
        cmocka_unit_test(decode_shall_reject_short_join_accept_with_cflist),        
        cmocka_unit_test(decode_shall_reject_long_join_accept),        
        cmocka_unit_test(decode_shall_reject_long_join_accept_with_cflist),        
        cmocka_unit_test(decode_shall_reject_unconfirmed_data_up),        
        cmocka_unit_test(decode_shall_reject_confirmed_data_up),        
        cmocka_unit_test(decode_shall_reject_join_request),        
        cmocka_unit_test(decode_shall_reject_rejoin_request)        
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
