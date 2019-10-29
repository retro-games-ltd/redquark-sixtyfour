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
#include <string.h>
#include <stdio.h>

// From vice/src/joystick.h
void joystick_set_value_absolute(unsigned int joyport, unsigned char value);

// --------------------------------------------------------------------------------
int joy_arch_init(void) { return 0; }
void joystick_close(void) { }
int joystick_arch_init_resources(void)  { return 0; }
int joystick_init_cmdline_options(void) { return 0; }

// --------------------------------------------------------------------------------

void emu_joystick_set( int port, unsigned char v ) {
    joystick_set_value_absolute( port, v );
}

// --------------------------------------------------------------------------------
