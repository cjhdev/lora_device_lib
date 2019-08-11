#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "lora_event.h"
#include "lora_system.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "mock_system_time.h"

/* setups */

static int setup_event(void **user)
{
    static struct lora_event state;
    LDL_Event_init(&state, NULL);
    *user = (void *)&state;                
    srand(time(0));
    system_time = rand();
    
    return 0;
}

/* expectations */

/* init ***************************************************************/

static void init_shall_init(void **user)
{
    assert_int_equal(0, setup_event(user));    
}

/* checkTimer *********************************************************/

static void checkTimer_shall_return_false_for_unset_timer(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    assert_false( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );
}

static void checkTimer_shall_return_true_for_immediate(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 0U);
    
    assert_true( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );
    assert_int_equal(0, error);    
}

static void checkTimer_shall_return_true_only_first_time_only(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 0U);
    
    assert_true( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );
    assert_int_equal(0, error);    
    
    assert_false( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );
}

static void checkTimer_shall_return_false_for_future(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 1U);
    
    assert_false( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );
}

static void checkTimer_shall_return_true_after_time_moves_forward(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 1U);
    
    assert_false( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );
    
    system_time++;
    
    assert_true( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );    
    assert_int_equal(0, error);        
}

static void checkTimer_shall_return_true_and_error_after_time_moves_forward(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 1U);
    
    system_time++;
    system_time++;
    system_time++;
    
    assert_true( LDL_Event_checkTimer(self, LORA_EVENT_WAITA, &error) );    
    assert_int_equal(2, error);        
}

/* checkInput ********************************************************/

static void checkInput_shall_return_false_for_no_signal(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_TIMEOUT, &error) );    
}

static void checkInput_shall_return_false_for_unarmed_signal(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_signal(self, LORA_EVENT_INPUT_RX_READY, system_time);
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );    
}

static void checkInput_shall_return_true_for_latched_signal_and_false_for_others(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setInput(self, LORA_EVENT_INPUT_RX_READY);
    
    LDL_Event_signal(self, LORA_EVENT_INPUT_RX_READY, system_time);
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
    assert_true( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_int_equal(0, error);        
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_TIMEOUT, &error) );    
}

static void checkInput_shall_return_time_error(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setInput(self, LORA_EVENT_INPUT_TX_COMPLETE);
    
    LDL_Event_signal(self, LORA_EVENT_INPUT_TX_COMPLETE, system_time);
    
    system_time++;
    system_time++;
    system_time++;
    system_time++;
    system_time++;
    
    assert_true( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
    assert_int_equal(5, error);        
}

static void checkInput_shall_return_true_only_first_time_only(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    LDL_Event_setInput(self, LORA_EVENT_INPUT_RX_READY);
    
    LDL_Event_signal(self, LORA_EVENT_INPUT_RX_READY, system_time);
    
    assert_true( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_int_equal(0, error);            
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
}


/* clearInput ********************************************************/

static void clearInput_shall_reset_inputs(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    uint32_t error;
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
    
    LDL_Event_setInput(self, LORA_EVENT_INPUT_TX_COMPLETE);
    LDL_Event_signal(self, LORA_EVENT_INPUT_TX_COMPLETE, system_time);
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_true( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
    
    LDL_Event_clearInput(self);
    
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
    
    LDL_Event_setInput(self, LORA_EVENT_INPUT_RX_READY);
    LDL_Event_signal(self, LORA_EVENT_INPUT_RX_READY, system_time);
    
    assert_true( LDL_Event_checkInput(self, LORA_EVENT_INPUT_RX_READY, &error) );
    assert_false( LDL_Event_checkInput(self, LORA_EVENT_INPUT_TX_COMPLETE, &error) );
}

/* ticksUntil ********************************************************/

static void ticksUntil_shall_return_max_for_never(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    assert_int_equal( UINT32_MAX, LDL_Event_ticksUntil(self, LORA_EVENT_INPUT_RX_READY) );
}

static void ticksUntil_shall_return_zero_for_immediate(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 0);
    
    assert_int_equal( 0, LDL_Event_ticksUntil(self, LORA_EVENT_WAITA) );
}

static void ticksUntil_shall_return_zero_for_past(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 0);
    
    system_time++;
    
    assert_int_equal( 0, LDL_Event_ticksUntil(self, LORA_EVENT_WAITA) );
}

static void ticksUntil_shall_return_positive_for_future(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 42);
    
    assert_int_equal( 42, LDL_Event_ticksUntil(self, LORA_EVENT_WAITA) );
}

/* ticksUntilNext *****************************************************/

static void ticksUntilNext_shall_return_max_for_never(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    assert_int_equal( UINT32_MAX, LDL_Event_ticksUntilNext(self) );
}

static void ticksUntilNext_shall_return_positive_for_future(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 42);
    
    assert_int_equal( 42, LDL_Event_ticksUntilNext(self) );
}

static void ticksUntilNext_shall_return_zero_for_interrupting_signal(void **user)
{
    struct lora_event *self = (struct lora_event *)(*user);    
    
    LDL_Event_setTimer(self, LORA_EVENT_WAITA, 42);
    
    LDL_Event_setInput(self, LORA_EVENT_INPUT_RX_READY);
    LDL_Event_signal(self, LORA_EVENT_INPUT_RX_READY, system_time);
    
    assert_int_equal( 0, LDL_Event_ticksUntilNext(self) );
}

/* runner */

int main(void)
{
    const struct CMUnitTest tests[] = {
        
        /* #init */
        cmocka_unit_test(
            init_shall_init
        ),
        
        /* checkTimer ********************************************************/
        
        cmocka_unit_test_setup(
            checkTimer_shall_return_false_for_future,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkTimer_shall_return_false_for_unset_timer,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkTimer_shall_return_true_after_time_moves_forward,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkTimer_shall_return_true_and_error_after_time_moves_forward,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkTimer_shall_return_true_for_immediate,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkTimer_shall_return_true_only_first_time_only,
            setup_event
        ),
        
        /* checkInput ********************************************************/
        
        cmocka_unit_test_setup(
            checkInput_shall_return_false_for_no_signal,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkInput_shall_return_false_for_unarmed_signal,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkInput_shall_return_true_for_latched_signal_and_false_for_others,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkInput_shall_return_time_error,
            setup_event
        ),
        cmocka_unit_test_setup(
            checkInput_shall_return_true_only_first_time_only,
            setup_event
        ),
        
        /* clearInput *************************************************/
        
        cmocka_unit_test_setup(
            clearInput_shall_reset_inputs,
            setup_event
        ),
        
        /* ticksUntil *************************************************/
      
        cmocka_unit_test_setup(
            ticksUntil_shall_return_max_for_never,
            setup_event
        ),
        cmocka_unit_test_setup(
            ticksUntil_shall_return_zero_for_immediate,
            setup_event
        ),
        cmocka_unit_test_setup(
            ticksUntil_shall_return_zero_for_past,
            setup_event
        ),
        cmocka_unit_test_setup(
            ticksUntil_shall_return_positive_for_future,
            setup_event
        ),
        
        /* ticksUntilNext *********************************************/
        
        cmocka_unit_test_setup(
            ticksUntilNext_shall_return_max_for_never,
            setup_event
        ),
        cmocka_unit_test_setup(
            ticksUntilNext_shall_return_positive_for_future,    
            setup_event
        ),
        cmocka_unit_test_setup(
            ticksUntilNext_shall_return_zero_for_interrupting_signal,
            setup_event
        ),
        
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
