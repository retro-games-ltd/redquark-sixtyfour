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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vice.h"
#include "uimenu.h"
#include "types.h"
#include "videoarch.h"
#include "uistatusbar.h"
#include "vsync.h"
#include "interrupt.h"
#include "machine.h"
#include "sound.h"
#include "sem.h"

void archdep_ui_init(int argc, char *argv[])
{
    return;
}


/* Main event handler */
ui_menu_action_t ui_dispatch_events(void)
{
    return MENU_ACTION_NONE;
}

void ui_check_mouse_cursor(void)
{
}

void ui_message(const char* format, ...)
{
}

void ui_sdl_quit(void)
{
}

/* Initialization  */
int ui_resources_init(void)
{
    return 0; // FIXME ??
}

void ui_resources_shutdown(void)
{
}

int ui_cmdline_options_init(void)
{
    return 0;
}

int ui_init(int *argc, char **argv)
{
    return 0;
}

int ui_init_finish(void)
{
    return 0;
}

int ui_init_finalize(void)
{
    return 0;
}

void ui_shutdown(void)
{
}

void ui_error(const char *format,...)
{
}

char* ui_get_file(const char *format,...)
{
    return NULL;
}

/* Drive related UI.  */
int ui_extend_image_dialog(void)
{
    return 0;
}

ui_jam_action_t ui_jam_dialog(const char *format, ...)
{
/*
    char *str;
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
*/
    
    return UI_JAM_NONE;
}

void ui_update_menus(void){}

int uicolor_alloc_color(unsigned int red, unsigned int green, unsigned int blue, unsigned long *color_pixel, BYTE *pixel_return)
{
    return 0;
}

void uicolor_free_color(unsigned int red, unsigned int green, unsigned int blue, unsigned long color_pixel)
{
}

void uicolor_convert_color_table(unsigned int colnr, BYTE *data, long color_pixel, void *c)
{
}

int uicolor_set_palette(struct video_canvas_s *c, const struct palette_s *palette)
{
    return 0;
}

void ui_cmdline_show_help(unsigned int num_options, void *options, void *userparam)
{
}

// ----------------------------------------------------------------------------------
