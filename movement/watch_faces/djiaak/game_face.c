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

#include <stdlib.h>
#include <string.h>
#include "game_face.h"
#include "watch.h"

const uint16_t SHOOT_COUNT = 10;
const uint16_t SHOOT_TOP_COM[] =    { 2, 1, 2, 1, 2, 2, 2, 2, 2, 1};
const uint16_t SHOOT_TOP_SEG[] =    { 5, 4, 3, 2,10, 0,23,22,21,17};
const uint16_t SHOOT_BOTTOM_COM[] = { 1, 0, 0, 0, 0, 1, 0, 1, 1, 0};
const uint16_t SHOOT_BOTTOM_SEG[] = { 6, 5, 4, 2, 1, 0,23,22,21,20};

void game_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        // in this case, we allocate an area of memory sufficient to store the stuff we need to track.
        *context_ptr = malloc(sizeof(game_state_t));
    }
}

void game_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    game_state_t *state = (game_state_t *)context;
    state->player_at_top = false;
    state->game_time = 0;

    movement_request_tick_frequency(16);
}

bool game_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    game_state_t *state = (game_state_t *)context;
    char buf[8];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            watch_clear_display();

            if (state->player_at_top) {
                watch_set_pixel(1, 19);
                watch_set_pixel(2, 18);
                watch_set_pixel(2, 19);
            } else {
                watch_set_pixel(1, 19);
                watch_set_pixel(0, 18);
                watch_set_pixel(0, 19);
            }
            
            watch_set_pixel(
                SHOOT_BOTTOM_COM[state->game_time % SHOOT_COUNT],
                SHOOT_BOTTOM_SEG[state->game_time % SHOOT_COUNT]
            );
            
            sprintf(buf, "%1d  %1d", (state->game_time / 10) % 10, state->game_time % 10);
            watch_display_string(buf, 0);
            state->game_time++;
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            state->player_at_top = !state->player_at_top;
            break;
    }
    
    return true;
}

void game_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
