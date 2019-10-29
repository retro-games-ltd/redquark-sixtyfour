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
#include "vice.h"

#include "console.h"
#include "lib.h"
#include "monitor.h"
#include "uimon.h"
//#include "ui.h"
#include "uimenu.h"


static console_t mon_console = {
    0,0,0,0,NULL
};


void uimon_window_close(void)
{
}

console_t *uimon_window_open(void)
{
    return &mon_console;
}

void uimon_window_suspend(void)
{
}

console_t *uimon_window_resume(void)
{
    return &mon_console;
}

int uimon_out(const char *buffer)
{
    return 0;
}

char *uimon_get_in(char **ppchCommandLine, const char *prompt)
{
    return "";
}

void uimon_notify_change(void)
{
}

void uimon_set_interface(monitor_interface_t **monitor_interface_init, int count)
{
}
