/*
 * MIT License
 *
 * Copyright (c) 2022 Jacques le Roux
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef AFR_CLOCK_FACE_H_
#define AFR_CLOCK_FACE_H_

#include "movement.h"

typedef struct {
    uint32_t previous_date_time;
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool signal_enabled;
    bool battery_low;
} afr_clock_state_t;

void afr_clock_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void afr_clock_face_activate(movement_settings_t *settings, void *context);
bool afr_clock_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void afr_clock_face_resign(movement_settings_t *settings, void *context);
bool afr_clock_face_wants_background_task(movement_settings_t *settings, void *context);

#define afr_clock_face ((const watch_face_t){ \
    afr_clock_face_setup, \
    afr_clock_face_activate, \
    afr_clock_face_loop, \
    afr_clock_face_resign, \
    afr_clock_face_wants_background_task, \
})

#endif // AFR_CLOCK_FACE_H_
