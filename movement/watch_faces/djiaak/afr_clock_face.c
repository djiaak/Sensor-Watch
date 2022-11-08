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
#include "afr_clock_face.h"
#include "watch.h"
#include "watch_utility.h"

static const char * afr_watch_utility_get_weekday(watch_date_time date_time) {
    static const char weekdays[7][3] = {"SA", "SO", "MA", "di", "WO", "do", "VR"};
    date_time.unit.year += 20;
    if (date_time.unit.month <= 2) {
        date_time.unit.month += 12;
        date_time.unit.year--;
    }
    return weekdays[(date_time.unit.day + 13 * (date_time.unit.month + 1) / 5 + date_time.unit.year + date_time.unit.year / 4 + 525) % 7];
}

void afr_clock_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(afr_clock_state_t));
        afr_clock_state_t *state = (afr_clock_state_t *)*context_ptr;
        state->signal_enabled = false;
        state->watch_face_index = watch_face_index;
    }
}

void afr_clock_face_activate(movement_settings_t *settings, void *context) {
    afr_clock_state_t *state = (afr_clock_state_t *)context;

    if (watch_tick_animation_is_running()) watch_stop_tick_animation();

    watch_clear_indicator(WATCH_INDICATOR_24H);
    if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_SIGNAL);

    watch_clear_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->previous_date_time = 0xFFFFFFFF;
}

bool afr_clock_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    afr_clock_state_t *state = (afr_clock_state_t *)context;
    char buf[11];
    uint8_t pos;

    watch_date_time date_time;
    uint32_t previous_date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = watch_rtc_get_date_time();
            previous_date_time = state->previous_date_time;
            state->previous_date_time = date_time.reg;

            // check the battery voltage once a day...
            if (date_time.unit.day != state->last_battery_check) {
                state->last_battery_check = date_time.unit.day;
                watch_enable_adc();
                uint16_t voltage = watch_get_vcc_voltage();
                watch_disable_adc();
                // 2.2 volts will happen when the battery has maybe 5-10% remaining?
                // we can refine this later.
                state->battery_low = (voltage < 2200);
            }

            // ...and set the LAP indicator if low.
            if (state->battery_low) watch_set_indicator(WATCH_INDICATOR_LAP);

            if ((date_time.reg >> 6) == (previous_date_time >> 6) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                // this face doesn't display seconds
                break;
            } else if ((date_time.reg >> 12) == (previous_date_time >> 12) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                // everything before minutes is the same.
                pos = 8;
                sprintf(buf, "%02d", date_time.unit.minute);
            } else {
                // other stuff changed; let's do it all.
                if (date_time.unit.hour < 12) {
                    watch_clear_indicator(WATCH_INDICATOR_PM);
                } else {
                    watch_set_indicator(WATCH_INDICATOR_PM);
                }
                date_time.unit.hour %= 12;
                pos = 0;
                static const char hour_names[12][5] = {"TWLF", "EEN ", "tWEE", "dRIE", "VIER", "Vy F", " SES", "SEWE", " AG ", "NEGE", "TIEN", " ELF"};

                sprintf(buf, "%s%2d%s%02d", afr_watch_utility_get_weekday(date_time), date_time.unit.day, hour_names[date_time.unit.hour],  date_time.unit.minute);
                
            }
            watch_display_string(buf, pos);
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            return false;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->signal_enabled = !state->signal_enabled;
            if (state->signal_enabled) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            else watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            if (watch_is_buzzer_or_led_enabled()) {
                // if we are in the foreground, we can just beep.
                movement_play_signal();
            } else {
                // if we were in the background, we need to enable the buzzer peripheral first,
                watch_enable_buzzer();
                // beep quickly (this call blocks for 275 ms),
                movement_play_signal();
                // and then turn the buzzer peripheral off again.
                watch_disable_buzzer();
            }
            break;
        default:
            break;
    }

    return true;
}

void afr_clock_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool afr_clock_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    afr_clock_state_t *state = (afr_clock_state_t *)context;
    if (!state->signal_enabled) return false;

    watch_date_time date_time = watch_rtc_get_date_time();

    return date_time.unit.minute == 0;
}