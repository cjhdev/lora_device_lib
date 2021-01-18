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

#ifndef MBED_LDL_RADIO_H
#define MBED_LDL_RADIO_H

#include "mbed.h"
#include "ldl_radio.h"
#include "ldl_system.h"

namespace LDL {

    /** Radio driver base class
     *
     * Users should instantiate one of the subclasses named for the
     * transciever or module they wish to drive.
     *
     * */
    class Radio {

        protected:

            static Radio *to_radio(void *self);

            Callback<void()> event_cb;

            static void _interrupt_handler(struct ldl_mac *self);

        public:

            void set_event_handler(Callback<void()> handler);

            virtual Radio *get_state();
            virtual const struct ldl_radio_interface *get_interface() = 0;
    };
};

#endif
