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

#include "mouse.h"
#include "vsyncapi.h"
#include "emumousedrv.h"

int mouse_x = 0, mouse_y = 0;
int mouse_accelx = 2, mouse_accely = 2;
static unsigned long mouse_timestamp = 0;

void mousedrv_mouse_changed(void)        { }
int  mousedrv_resources_init(void)       { return 0; }
int  mousedrv_cmdline_options_init(void) { return 0; }
void mousedrv_init(void) { }

int mousedrv_get_x(void)
{
    return mouse_x;
}

int mousedrv_get_y(void)
{
    return mouse_y;
}

void mouse_move( int dx, int dy )
{
    mouse_x += dx;
    mouse_y -= dy;
    mouse_x &= 65535;
    mouse_y &= 65535;
    mouse_timestamp = vsyncarch_gettime();
}

unsigned long mousedrv_get_timestamp(void)
{
    return mouse_timestamp;
}
