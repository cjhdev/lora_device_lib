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

/* timerCheck *********************************************************/

static void timerCheck_shall_return_false_for_unset_timer(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    assert_false( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );
}

static void timerCheck_shall_return_true_for_immediate(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0U);
    
    assert_true( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );
    assert_int_equal(0, error);    
}

static void timerCheck_shall_return_true_only_first_time_only(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0U);
    
    assert_true( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );
    assert_int_equal(0, error);    
    
    assert_false( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );
}

static void timerCheck_shall_return_false_for_future(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 1U);
    
    assert_false( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );
}

static void timerCheck_shall_return_true_after_time_moves_forward(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 1U);
    
    assert_false( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );
    
    system_time++;
    
    assert_true( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );    
    assert_int_equal(0, error);        
}

static void timerCheck_shall_return_true_and_error_after_time_moves_forward(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 1U);
    
    system_time++;
    system_time++;
    system_time++;
    
    assert_true( LDL_MAC_timerCheck(self, LDL_TIMER_WAITA, &error) );    
    assert_int_equal(2, error);        
}

/* timerTicksUntil ********************************************************/

static void timerTicksUntil_shall_return_max_for_never(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    assert_int_equal( UINT32_MAX, LDL_MAC_timerTicksUntil(self, LDL_TIMER_WAITA, &error) );
}

static void timerTicksUntil_shall_return_zero_for_immediate(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user); 
    uint32_t error;   
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
    
    assert_int_equal( 0, LDL_MAC_timerTicksUntil(self, LDL_TIMER_WAITA, &error) );
}

static void timerTicksUntil_shall_return_zero_for_past(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 0);
    
    system_time++;
    
    assert_int_equal( 0, LDL_MAC_timerTicksUntil(self, LDL_TIMER_WAITA, &error) );
}

static void timerTicksUntil_shall_return_positive_for_future(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    uint32_t error;
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 42);
    
    assert_int_equal( 42, LDL_MAC_timerTicksUntil(self, LDL_TIMER_WAITA, &error) );
}

/* timerTicksUntilNext *****************************************************/

static void timerTicksUntilNext_shall_return_max_for_never(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    
    assert_int_equal( UINT32_MAX, LDL_MAC_timerTicksUntilNext(self) );
}

static void timerTicksUntilNext_shall_return_positive_for_future(void **user)
{
    struct ldl_mac *self = (struct ldl_mac *)(*user);    
    
    LDL_MAC_timerSet(self, LDL_TIMER_WAITA, 42);
    
    assert_int_equal( 42, LDL_MAC_timerTicksUntilNext(self) );
}

/* runner */

int main(void)
{
    const struct CMUnitTest tests[] = {
        
        /* timerCheck ********************************************************/
        
        cmocka_unit_test_setup(
            timerCheck_shall_return_false_for_future,
            setup
        ),
        cmocka_unit_test_setup(
            timerCheck_shall_return_false_for_unset_timer,
            setup
        ),
        cmocka_unit_test_setup(
            timerCheck_shall_return_true_after_time_moves_forward,
            setup
        ),
        cmocka_unit_test_setup(
            timerCheck_shall_return_true_and_error_after_time_moves_forward,
            setup
        ),
        cmocka_unit_test_setup(
            timerCheck_shall_return_true_for_immediate,
            setup
        ),
        cmocka_unit_test_setup(
            timerCheck_shall_return_true_only_first_time_only,
            setup
        ),
        
        
        /* timerTicksUntil *************************************************/
      
        cmocka_unit_test_setup(
            timerTicksUntil_shall_return_max_for_never,
            setup
        ),
        cmocka_unit_test_setup(
            timerTicksUntil_shall_return_zero_for_immediate,
            setup
        ),
        cmocka_unit_test_setup(
            timerTicksUntil_shall_return_zero_for_past,
            setup
        ),
        cmocka_unit_test_setup(
            timerTicksUntil_shall_return_positive_for_future,
            setup
        ),
        
        /* timerTicksUntilNext *********************************************/
        
        cmocka_unit_test_setup(
            timerTicksUntilNext_shall_return_max_for_never,
            setup
        ),
        cmocka_unit_test_setup(
            timerTicksUntilNext_shall_return_positive_for_future,    
            setup
        )                
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
