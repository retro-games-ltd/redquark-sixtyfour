/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef THE64_UI_STATUS_BAR
#define THE64_UI_STATUS_BAR

#include "vice.h"
#include <stdio.h>
#include "types.h"
#include "uimenu.h"

void ui_display_speed(float percent, float framerate, int warp_flag);
void ui_display_paused(int flag);
void ui_display_statustext(const char *text, int fade_out);
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color);
void ui_display_drive_track(unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number);
void ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int pwm2);
void ui_display_drive_current_image(unsigned int drive_number, const char *image);
void ui_set_tape_status(int tape_status);
void ui_display_tape_motor_status(int motor);
void ui_display_tape_control_status(int control);
void ui_display_tape_counter(int counter);
void ui_display_tape_current_image(const char *image);
void ui_display_playback(int playback_status, char *version);
void ui_display_recording(int recording_status);
void ui_display_event_time(unsigned int current, unsigned int total);
void ui_display_joyport(BYTE *joyport);
void ui_display_volume(int vol);
int uistatusbar_init_resources(void);
void uistatusbar_open(void);
void uistatusbar_close(void);
void uistatusbar_draw(void);

#endif
