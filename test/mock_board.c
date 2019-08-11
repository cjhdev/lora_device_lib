#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include "cmocka.h"
#include "lora_radio.h"
#include "mock_board.h"
#include "lora_board.h"

void board_select(void *receiver, bool state)
{
    struct mock_chip *self = (struct mock_chip *)receiver;
    
    if(state){
        
        assert_false(self->select);  //double select        
    }
    else{
        
        assert_true(self->select);  //double select        
    }
    
    check_expected(state);
    
    self->select = state;    
}

void board_reset(void *receiver, bool state)
{
    struct mock_chip *self = (struct mock_chip *)receiver;
    
    if(state){
        
        assert_false(self->reset);  //double reset        
    }
    else{
        
        assert_true(self->reset);  //double reset        
    }
    
    check_expected(state);
    
    self->reset = state;    
}

void board_write(void *receiver, uint8_t data)
{
    struct mock_chip *self = (struct mock_chip *)receiver;
    
    assert_true(self->select);
    assert_false(self->reset);    
    
    check_expected(data);
}

uint8_t board_read(void *receiver)
{
    struct mock_chip *self = (struct mock_chip *)receiver;
    
    assert_true(self->select);
    assert_false(self->reset);
    
    return mock();    
}


