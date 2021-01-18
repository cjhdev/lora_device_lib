/* Copyright (c) 2020 Cameron Harper
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * */

#include "radio.h"
#include "ldl_system.h"
#include "ldl_radio.h"

using namespace LDL;

/* constructors *******************************************************/

/* static protected ***************************************************/

Radio *
Radio::to_radio(void *self)
{
    return static_cast<Radio *>(self);
}

void
Radio::_interrupt_handler(struct ldl_mac *self)
{
    if(to_radio(self)->event_cb){

        to_radio(self)->event_cb();
    }
}

/* protected **********************************************************/


/* public *************************************************************/

void
Radio::set_event_handler(Callback<void()> handler)
{
    event_cb = handler;
}

Radio *
Radio::get_state()
{
    return this;
}
