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
#ifndef VICE_UIMENU_H
#define VICE_UIMENU_H

#include "vice.h"

typedef void* ui_callback_data_t;
typedef const char *(*ui_callback_t)(int activated, ui_callback_data_t param);

typedef enum {
    Fred
} ui_menu_entry_type_t;
typedef enum {
    MENU_ACTION_NONE
} ui_menu_action_t;

typedef struct ui_menu_entry_s {
    char *string;
    ui_menu_entry_type_t type;
    ui_callback_t callback;
    ui_callback_data_t data;
} ui_menu_entry_t;

typedef enum {
    MENU_RETVAL_DEFAULT,
    MENU_RETVAL_EXIT_UI
} ui_menu_retval_t;


typedef enum {
        UI_JAM_RESET, UI_JAM_HARD_RESET, UI_JAM_MONITOR, UI_JAM_NONE
} ui_jam_action_t;

typedef enum {
        UI_DRIVE_ENABLE_NONE = 0,
            UI_DRIVE_ENABLE_0 = 1 << 0,
                UI_DRIVE_ENABLE_1 = 1 << 1,
                    UI_DRIVE_ENABLE_2 = 1 << 2,
                        UI_DRIVE_ENABLE_3 = 1 << 3
} ui_drive_enable_t;

#endif
