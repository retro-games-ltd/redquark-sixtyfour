/*
 *  THEC64 Mini
 *  Copyright (C) 2021 Chris Smith
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
#ifndef EMU_CORE_MOUSEDRV_H
#define EMU_CORE_MOUSEDRV_H

typedef enum {
    EMU_MOUSE_NONE,
    EMU_MOUSE_AXIS,
    EMU_MOUSE_BUTTON_LEFT,
    EMU_MOUSE_BUTTON_RIGHT,
    EMU_MOUSE_BUTTON_MIDDLE,
    EMU_MOUSE_BUTTON_WHEEL_UP,
    EMU_MOUSE_BUTTON_WHEEL_DOWN,
} emu_mouse_control;

void mouse_move( int x, int y );

#endif
