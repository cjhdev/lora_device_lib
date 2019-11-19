#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include "cmocka.h"

#include "ldl_mac.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

extern uint32_t system_time;

/* setups */

static int setup(void **user)
{
    static struct ldl_mac state;
    (void)memset(&state, 0, sizeof(state));
    *user = (void *)&state;                
    
    return 0;
}

/* expectations */

/* inputCheck ********************************************************/

static void inputCheck_shall_return_false_for_no_signal(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_TIMEOUT, &error) );    
}

static void inputCheck_shall_return_false_for_unarmed_signal(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_inputSignal(self, LDL_INPUT_RX_READY);
    
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );    
}

static void inputCheck_shall_return_true_for_latched_signal_and_false_for_others(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_inputArm(self, LDL_INPUT_RX_READY);
    
    LDL_MAC_inputSignal(self, LDL_INPUT_RX_READY);
    
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
    assert_true( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );
    assert_int_equal(0, error);        
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_TIMEOUT, &error) );    
}

static void inputCheck_shall_return_time_error(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_inputArm(self, LDL_INPUT_TX_COMPLETE);
    
    LDL_MAC_inputSignal(self, LDL_INPUT_TX_COMPLETE);
    
    system_time++;
    system_time++;
    system_time++;
    system_time++;
    system_time++;
    
    assert_true( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
    assert_int_equal(5, error);        
}


/* inputClear ********************************************************/

static void inputClear_shall_reset_inputs(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
    
    LDL_MAC_inputArm(self, LDL_INPUT_TX_COMPLETE);
    LDL_MAC_inputSignal(self, LDL_INPUT_TX_COMPLETE);
    
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );
    assert_true( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
    
    LDL_MAC_inputClear(self);
    
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
    
    LDL_MAC_inputArm(self, LDL_INPUT_RX_READY);
    LDL_MAC_inputSignal(self, LDL_INPUT_RX_READY);
    
    assert_true( LDL_MAC_inputCheck(self, LDL_INPUT_RX_READY, &error) );
    assert_false( LDL_MAC_inputCheck(self, LDL_INPUT_TX_COMPLETE, &error) );
}

/* runner */

int main(void)
{
    const struct CMUnitTest tests[] = {
        
        /* inputCheck ********************************************************/
        
        cmocka_unit_test_setup(
            inputCheck_shall_return_false_for_no_signal,
            setup
        ),
        cmocka_unit_test_setup(
            inputCheck_shall_return_false_for_unarmed_signal,
            setup
        ),
        cmocka_unit_test_setup(
            inputCheck_shall_return_true_for_latched_signal_and_false_for_others,
            setup
        ),
        cmocka_unit_test_setup(
            inputCheck_shall_return_time_error,
            setup
        ),
        
        /* inputClear *************************************************/
        
        cmocka_unit_test_setup(
            inputClear_shall_reset_inputs,
            setup
        ),        
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
