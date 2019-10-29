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
#ifndef PORTUI_H
#define PORTUI_H

int emu_pause(int flag);
int emu_can_change_pause_state();
int emu_is_paused();

void emu_load_snapshot( unsigned char *filename );
void emu_save_snapshot( unsigned char *filename );

void disable_pause_and_wait_for_release();
int schedule_emulator_quit();

#endif
