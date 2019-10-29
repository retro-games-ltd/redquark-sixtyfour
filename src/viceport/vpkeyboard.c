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
#include <stdlib.h>
#include <stdio.h>

#include "keyboard.h"

void kbd_arch_init(void)
{
    keyboard_shiftlock = 0;
}

signed long kbd_arch_keyname_to_keynum(char *keyname)
{
    // vkm file contains actual evdev key codes not names
    return (signed long)atoi(keyname);
}

const char *kbd_arch_keynum_to_keyname(signed long keynum)
{
    static char keyname[20] = {0};
    sprintf( keyname, "%ld", keynum );
    return keyname;
}

void kbd_initialize_numpad_joykeys(int* joykeys) { }

void emu_key_press(signed long key) {
    keyboard_key_pressed(key);
}

void emu_key_release(signed long key) {
    keyboard_key_released(key);
}

// Delegated to emu cores, since functionality differs between them
// void emu_key_capslock( int state )
//{
//    keyboard_shiftlock = state ? 1 : 0;
//    keyboard_set_keyarr_any(1, 7, keyboard_shiftlock );  // 1, 3 for Vic20
//}
