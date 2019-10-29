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
#include "videoarch.h"
#include "video.h"
#include "palette.h"
#include "viewport.h"
#include "video/render1x1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static video_canvas_t *cached_video_canvas = NULL;
static int v_adjust = 0;  // +ve means move up. -ve down  ( -16 <= adjust <= 16 )

int  video_arch_resources_init    (void) { return 0; }
void video_arch_resources_shutdown(void) { }

int  video_init_cmdline_options   (void) { return 0; }
int  video_init                   (void) { return 0; }
void video_shutdown               (void) { }
void video_add_handlers           (void) { }

void video_canvas_resize          (struct          video_canvas_s *canvas, char resize_canvas) { }
void video_canvas_destroy         (struct          video_canvas_s *canvas) { }
char video_canvas_can_resize      (video_canvas_t *canvas ) { return 1; }

// ------------------------------------------------------------------------------------------
// This is called repeatedly to render dirty rectangles
// XXX This is disable for reduqark, by link wrapping the follwing functions:
// XXX   void raster_canvas_handle_end_of_frame( void *raster )
// XXX   void video_canvas_refresh_all         ( void *canvas )
// XXX See src/ui/Makefile for
// XXX    -Wl,-wrap=raster_canvas_handle_end_of_frame -Wl,-wrap=video_canvas_refresh_all
// XXX
// XXX Nop __wrap_* function are at the end of this file.
// XXX
// XXX Note, video_canvas_refresh_all(), and hence videp_canvas_refresh is called when
// XXX setting the palette, the actual call being within:
// XXX    vice/src/video/video-canvas.c:video_canvas_palette_set()
// XXX but this only happens once.
void 
video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
    //printf("\n*** video_canvas_refresh - Should not be called (except once, by video_canvas_palette_set) **\n" );
    return;
}

// ------------------------------------------------------------------------------------------
//
int 
video_canvas_set_palette(struct video_canvas_s *canvas, struct palette_s *palette)
{
    unsigned int i, col;
    if (palette == NULL) return 0; // Palette not create yet.

    for (i = 0; i < palette->num_entries; i++) {
        //printf("%02x %02x %02x 0\n", palette->entries[i].red, palette->entries[i].green, palette->entries[i].blue); // Dump out palette

        col = (palette->entries[i].red   <<   RED_SHIFT) | 
              (palette->entries[i].green << GREEN_SHIFT) |
              (palette->entries[i].blue  <<  BLUE_SHIFT) | 0xff000000; // ff is the alpha

        video_render_setphysicalcolor(canvas->videoconfig, i, col, canvas->depth);
    }

    canvas->palette = palette;

    return 0;
}

// --------------------------------------------------------------------------
//
void __wrap_raster_canvas_handle_end_of_frame( void *raster ) { };
void __wrap_video_canvas_refresh_all         ( void *canvas ) { };

// --------------------------------------------------------------------------
