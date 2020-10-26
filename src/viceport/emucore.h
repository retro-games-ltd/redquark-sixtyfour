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
#ifndef EMU_CORE_H
#define EMU_CORE_H

#include "autostart.h"
#include "joystick.h"

#include "videoarch.h"

#include "emu_bind_decl.h"

// core.c
int  emu_initialise();
int  emu_start();
void emu_stop();
void emu_set_model_ntsc();
void emu_set_model_pal();
void emu_configure_video_60hz();
void emu_configure_video_50hz();
void emu_wait_for_frame();
void emu_start_frame();
void emu_core_reset( emu_reset_type_t rt );

void emu_set_vertical_shift( int adjust );

void core_cartridge_attach_image( const char *filename );
void core_cartridge_trigger_freeze();

void vsyncarch_sync_with_raster(video_canvas_t *c);
int delete_temporary_files();

#endif
