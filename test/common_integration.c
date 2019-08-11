#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_mac.h"
#include "lora_board.h"

static const uint8_t eui[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
static const uint8_t key[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

static void responseHandler(void *receiver, enum lora_mac_response_type type, const union lora_mac_response_arg *arg)
{
}

static void board_select(void *receiver, bool state)
{
}
static void board_reset(void *receiver, bool state)
{
}
static uint8_t board_read(void *receiver)
{
    return 0U;
}
static void board_write(void *receiver, uint8_t data)
{
}

static void test_init(void **user)
{
    struct lora_mac self;
    struct lora_board board;
    struct lora_radio radio;
    
    LDL_Board_init(&board, 
        NULL, 
        board_select,
        board_reset,
        board_write,
        board_read
    );

    LDL_Radio_init(&radio, LORA_RADIO_SX1272, &board);
    LDL_MAC_init(&self, &self, EU_863_870, &radio, responseHandler);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
