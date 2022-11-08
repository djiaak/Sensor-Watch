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

 // Emulator only: need time() to seed the random number generator.
#if __EMSCRIPTEN__
#include <time.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "game_face.h"
#include "watch.h"

const uint16_t SHOOT_COUNT = 10;
const uint16_t SHOOT_TOP_COM[] =    { 2, 1, 2, 1, 2, 2, 2, 2, 2, 1};
const uint16_t SHOOT_TOP_SEG[] =    { 5, 4, 3, 2,10, 0,23,22,21,17};
const uint16_t SHOOT_BOTTOM_COM[] = { 1, 0, 0, 0, 0, 1, 0, 1, 1, 0};
const uint16_t SHOOT_BOTTOM_SEG[] = { 6, 5, 4, 2, 1, 0,23,22,21,20};

const uint16_t BULLET_SPAWN_PROB = 50;
const uint16_t TOTAL_GAME_TIME = 500;
const uint16_t COUNTER_MAX = 39;
const uint16_t TICK_PER_UPDATE_INC_INTERVAL = 100;
const uint16_t TICKS_PER_UPDATE_INITIAL = 5;

static uint8_t generate_random_number(uint8_t max) {
    // Emulator: use rand. Hardware: use arc4random.
    #if __EMSCRIPTEN__
    return rand() % max;
    #else
    return arc4random_uniform(max);
    #endif
}



static void beep() {
    watch_buzzer_play_note(BUZZER_NOTE_A7, 20);
}


static void play_intro() {
    watch_display_string("     CATCH", 0);
    watch_buzzer_play_note(BUZZER_NOTE_C7, 500);
    watch_display_string("     THE    ", 0);
    watch_buzzer_play_note(BUZZER_NOTE_D7, 500);
    watch_display_string("     LINES", 0);
    watch_buzzer_play_note(BUZZER_NOTE_E7, 800);
}

static void play_outro() {
    watch_buzzer_play_note(BUZZER_NOTE_G7, 500);
    watch_buzzer_play_note(BUZZER_NOTE_E7, 500);
    watch_buzzer_play_note(BUZZER_NOTE_B6, 300);
    watch_buzzer_play_note(BUZZER_NOTE_C7, 400);
}

static void update_bullets(game_state_t *state) {
    uint16_t max = 0;
    for (uint16_t i=0; i<MAX_BULLETS_PER_LINE; i++) {
        if (state->bullets_top[i]) {
            state->bullets_top[i]--;
            if (state->bullets_top[i] == 0 && state->player_at_top) {
                state->score++;
                watch_set_led_red();
                beep();
                watch_set_led_off();
            }
        }
        if (state->bullets_bottom[i]) {
            state->bullets_bottom[i]--;
            if (state->bullets_bottom[i] == 0 && !state->player_at_top) {
                state->score++;
                watch_set_led_green();
                beep();
                watch_set_led_off();
            }
        }
        if (state->bullets_top[i] > max) max = state->bullets_top[i];
        if (state->bullets_bottom[i] > max) max = state->bullets_bottom[i];
    }

    uint8_t rnd = generate_random_number(100);
    uint16_t *arr = (rnd % 2) ? state->bullets_top : state->bullets_bottom;
    if (max < SHOOT_COUNT-2 && rnd < BULLET_SPAWN_PROB) {
        for (uint16_t i=0; i<MAX_BULLETS_PER_LINE; i++) {
            if (!arr[i]) {
                arr[i] = SHOOT_COUNT;
                break;
            }
        }
    }
}

static void draw_bullets(game_state_t *state) {
    for (uint16_t i=0; i<MAX_BULLETS_PER_LINE; i++) {
        if (state->bullets_top[i]) {
            watch_set_pixel(
                SHOOT_TOP_COM[SHOOT_COUNT - state->bullets_top[i]],
                SHOOT_TOP_SEG[SHOOT_COUNT - state->bullets_top[i]]
            );
        }
        if (state->bullets_bottom[i]) {
            watch_set_pixel(
                SHOOT_BOTTOM_COM[SHOOT_COUNT - state->bullets_bottom[i]],
                SHOOT_BOTTOM_SEG[SHOOT_COUNT - state->bullets_bottom[i]]
            );
        }
    }
}

void game_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        // in this case, we allocate an area of memory sufficient to store the stuff we need to track.
        *context_ptr = malloc(sizeof(game_state_t));
    }
    // Emulator only: Seed random number generator
    #if __EMSCRIPTEN__
    srand(time(NULL));
    #endif
}

void game_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    memset(context, 0, sizeof(game_state_t));
    game_state_t *state = (game_state_t *)context;
    state->state = START;
    state->game_time = 0;

    movement_request_tick_frequency(32);
}


bool game_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    game_state_t *state = (game_state_t *)context;
    char buf[8];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            if (state->state == START) {
                if ((state->game_time/40) % 2) {
                    watch_display_string("     PUSH ", 0);
                } else {
                    watch_display_string("     BUTUN ", 0);
                }
            }
            else if (state->state == INTRO) {
                play_intro();
                state->state = PLAYING;
                state->player_at_top = false;
                state->game_time = 0;
                state->score = 0;
                state->ticks_per_update = TICKS_PER_UPDATE_INITIAL;
            } else if (state->state == PLAYING) {
                watch_clear_display();
                
                uint8_t time = COUNTER_MAX - (state->game_time * COUNTER_MAX / TOTAL_GAME_TIME);
                sprintf(buf, "  %2d", time);
                watch_display_string(buf, 0);

                if (state->player_at_top) {
                    watch_set_pixel(1, 19);
                    watch_set_pixel(2, 18);
                    watch_set_pixel(2, 19);
                } else {
                    watch_set_pixel(1, 19);
                    watch_set_pixel(0, 18);
                    watch_set_pixel(0, 19);
                }

                if (state->game_time % state->ticks_per_update == 0) {
                    update_bullets(state);
                }
               
                if ((state->game_time % TICK_PER_UPDATE_INC_INTERVAL == 0) && state->ticks_per_update > 1) {
                    state->ticks_per_update--;
                }
                if (state->game_time >= TOTAL_GAME_TIME) {
                    state->state = DONE;
                }


                draw_bullets(state);
                
            } else if (state->state == DONE) {
               watch_clear_display();
               sprintf(buf, "%3d", state->score);
               watch_display_string(buf, 5);
               play_outro();
               state->state = SCORE;
            }

            state->game_time++;           
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            if (state->state == START || state->state == SCORE) {
                state->state = INTRO;
            } else if (state->state == PLAYING) {
                state->player_at_top = !state->player_at_top;
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            return false;
    }
    
    return true;
}



void game_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
