#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include "cmocka.h"
#include "ldl_radio.h"
#include "mock_ldl_chip.h"
#include "ldl_chip.h"

void LDL_Chip_select(void *self, bool state)
{
    struct mock_lora_chip *_self = (struct mock_lora_chip *)self;
    
    if(state){
        
        assert_false(_self->select);  //double select        
    }
    else{
        
        assert_true(_self->select);  //double select        
    }
    
    check_expected(state);
    
    _self->select = state;    
}

void LDL_Chip_reset(void *self, bool state)
{
    struct mock_lora_chip *_self = (struct mock_lora_chip *)_self;
    
    if(state){
        
        assert_false(_self->reset);  //double reset        
    }
    else{
        
        assert_true(_self->reset);  //double reset        
    }
    
    check_expected(state);
    
    _self->reset = state;    
}

void LDL_Chip_write(void *self, uint8_t data)
{
    struct mock_lora_chip *_self = (struct mock_lora_chip *)self;
    
    assert_true(_self->select);
    assert_false(_self->reset);    
    
    check_expected(data);
}

uint8_t LDL_Chip_read(void *self)
{
    struct mock_lora_chip *_self = (struct mock_lora_chip *)self;
    
    assert_true(_self->select);
    assert_false(_self->reset);
    
    return mock();    
}
