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
void mousedrv_mouse_changed(void)        { }
int  mousedrv_resources_init(void)       { return 0; }
int  mousedrv_cmdline_options_init(void) { return 0; }
void mousedrv_init(void)                 { }

void mouse_button(int bnumber, int state) { }

int mousedrv_get_x(void) { return 0; }

int mousedrv_get_y(void) { return 0; }

void mouse_move(float dx, float dy) { }

unsigned long mousedrv_get_timestamp(void) { return 0; }
