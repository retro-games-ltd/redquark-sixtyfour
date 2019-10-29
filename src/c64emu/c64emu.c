/*
 *  THEC64 Mini
 *  Copyright (C) 2019 Chris Smith
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
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

#include "videoarch.h"
#include "machine.h"
#include "sem.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include "emucore.h"

// Vice includes
#include "vdrive/vdrive-internal.h"
#include "attach.h"
#include "diskimage.h"
#include "resources.h"
#include "c64/c64model.h"
#include "c64/c64.h"
#include "cartridge.h"
#include "sid/sid.h"
#include "sound.h"
#include "palette.h"
#include "video.h"
#include "keyboard.h"
#include "../ui/usb.h"
#include "../ui/emu_bind_decl.h"
#include "../ui/machine_model.h"
#include "emucore.h"

#ifndef GLOBAL_OUTPUT
#   ifdef PAL_OUTPUT
#      warning PAL Output
#   else
#      warning NTSC Output
#   endif
#endif

#define NORMAL_START_LINE           31
#define NORMAL_VISIBLE_HEIGHT       240
#define  PAL_NORMAL_START_LINE      NORMAL_START_LINE      // Can be shifted -15 max +17
#define  PAL_NORMAL_VISIBLE_HEIGHT  NORMAL_VISIBLE_HEIGHT
#define NTSC_NORMAL_START_LINE      NORMAL_START_LINE      // Can be shifted -15 max +17
#define NTSC_NORMAL_VISIBLE_HEIGHT  NORMAL_VISIBLE_HEIGHT

#define  PAL_FULL_START_LINE         16     //  31 - 15
#define  PAL_FULL_VISIBLE_HEIGHT    272     // 240 + 32
#define NTSC_FULL_START_LINE         28
#define NTSC_FULL_VISIBLE_HEIGHT    247

#define PAL_LINES           C64_PAL_SCREEN_LINES        // 312
#define PAL_LINE_CLKS       C64_PAL_CYCLES_PER_LINE     // 63
#define PAL_FRAME_CLKS      (PAL_LINES * PAL_LINE_CLKS) // 312 * 63
#define PAL_CLKS_SECOND_TRUE (PAL_FRAME_CLKS * 50)

#define NTSC_LINES          C64_NTSC_SCREEN_LINES        // 263
#define NTSC_LINE_CLKS      C64_NTSC_CYCLES_PER_LINE     // 65
#define NTSC_FRAME_CLKS     (NTSC_LINES * NTSC_LINE_CLKS) // 263 * 65
#define NTSC_CLKS_SECOND_TRUE (NTSC_FRAME_CLKS * 60)

#define BASIC_DISK_IMAGE_NAME "THEC64-drive8.d64"

static video_canvas_t *cached_video_canvas = NULL;
static int v_adjust = 0;  // +ve means move up. -ve down  ( -15 <= adjust <= 17 )

static int  screen_start_line  = NORMAL_START_LINE;
static int  visible_lines      = NORMAL_VISIBLE_HEIGHT;

int emu_get_model();

static long pal_clks_second  = PAL_CLKS_SECOND_TRUE;
static long ntsc_clks_second = NTSC_CLKS_SECOND_TRUE;

static int  video_output_60  = 1;

static void _configure_video_50hz();
static void _configure_video_60hz();

static int             is_model_configured = 0;
static machine_model_t configured_model    = 0;
static int sound_scale_enabled = 1;

//static struct video_canvas_s *local_canvas = NULL;

emu_capabilities_t * emu_capabilities();

// -------------------------------------------------------------------------------
// Core specific initialisation
int
core_init()
{
    // Turning sound warp mode on prevents start-up clicks when Vice starts up
    // its sound core.
    sound_set_warp_mode(1);

    // Prevent VICE from using the raw filesystem as a Disk
    resources_set_int("FileSystemDevice8",  0 );
    resources_set_int("FileSystemDevice9",  0 );
    resources_set_int("FileSystemDevice10", 0 );
    resources_set_int("FileSystemDevice11", 0 );
    resources_set_int("AutostartPrgMode",   1 ); // Handle PRG as injection
    //resources_set_int("AutostartPrgMode",   2 ); // Handle PRG's as though they on their own disk image (AUTOSTART_PRG_MODE_DISK)
    //resources_set_string("AutostartPrgDiskImage", "/tmp/prgdisk.d64" ); // Blank disk to create/use to hadle PRG loading

    resources_set_int("AutostartDelay", 3 ); // Use the default of 3

    resources_set_int("SidEngine", SID_ENGINE_RESID );
    resources_set_int("SidModel" , SID_MODEL_6581 );
    resources_set_int("SidResidSampling", 0); // Fast

    resources_set_int("RAMInitStartValue", 0 );

    resources_set_string("VICIIPaletteFile", "theC64-palette.vpl" ); // Load is actually deferred....
    resources_set_int   ("VICIIExternalPalette", 1);

    is_model_configured = 0;
    configured_model = Model_Video_Type_PAL;
}

// -------------------------------------------------------------------------------
// Core specific startup
int
core_start()
{
//    if( video_output_60 ) _configure_video_60hz();
//    else                  _configure_video_50hz();
}

// -------------------------------------------------------------------------------
// This is not safe to call before emulation core init has finished
// So called by core_start
static void
_configure_video_50hz()
{

    ntsc_clks_second = NTSC_FRAME_CLKS * 50;
    pal_clks_second  =  PAL_FRAME_CLKS * 50;

    // When TheC64 runs the display at 50Hz, we have to do some magic to get 
    // the SID generating 882 samples (44100/50) per frame.
    // This is irrespective of the emulated C64 machine.
    machine_set_sound_cycles(MACHINE_SYNC_PAL,   pal_clks_second,  PAL_FRAME_CLKS );
    machine_set_sound_cycles(MACHINE_SYNC_NTSC, ntsc_clks_second, NTSC_FRAME_CLKS );
}

// -------------------------------------------------------------------------------
// This is not safe to call before emulation core init has finished
// So called by core_start
static void
_configure_video_60hz()
{
    ntsc_clks_second = NTSC_FRAME_CLKS * 60;
    pal_clks_second  =  PAL_FRAME_CLKS * 60;

    // When running at 60Hz, we need to generate 735 (44100/60) samples per frame.
    // This is irrespective of the emulated C64 machine.
    machine_set_sound_cycles(MACHINE_SYNC_PAL,   pal_clks_second,  PAL_FRAME_CLKS );
    machine_set_sound_cycles(MACHINE_SYNC_NTSC, ntsc_clks_second, NTSC_FRAME_CLKS );
}

// -------------------------------------------------------------------------------
// Sets up video parameters to take affect at next emu->start
//
void
emu_configure_video_50hz()
{
    video_output_60 = 0;
    _configure_video_50hz();
}

// -------------------------------------------------------------------------------
// Sets up video parameters to take affect at next emu->start
//
void
emu_configure_video_60hz()
{
    video_output_60 = 1;
    _configure_video_60hz();
}

// -------------------------------------------------------------------------------
//
void
emu_sound_scale_enable( int e )
{
    sound_scale_enabled = e ? 1 : 0;

    float freq_scale = 1.0;

    if( sound_scale_enabled ) {
        int m = emu_get_model();

        if( m == -1 ) return;

        float d;
        if( m & Model_C64_PAL ) {
            d = pal_clks_second - PAL_CLKS_SECOND_TRUE;
            //freq_scale = 1.0 - d / (d >= 0 ? pal_clks_second : PAL_CLKS_SECOND_TRUE); // d is always >= 0
            freq_scale = 1.0 - d / pal_clks_second;
        } else {
            d = ntsc_clks_second - NTSC_CLKS_SECOND_TRUE;
            //freq_scale = 1.0 - d / (d < 0 ? ntsc_clks_second : NTSC_CLKS_SECOND_TRUE); // d is always <= 0
            freq_scale = 1.0 - d / ntsc_clks_second;
        }
    }
    sound_set_audio_scaling( freq_scale );
}

// -------------------------------------------------------------------------------
static void
set_model_ntsc( emu_screen_t *sc )
{

    // This will cause a HARD reset
    resources_set_int("MachineVideoStandard", MACHINE_SYNC_NTSC );

    screen_start_line = sc->_start_line;
    visible_lines     = sc->pixel_height;

    // Apply audio frequency scaling to restore the original frequency 
    // since it will be shifted down when emulating NTSC @ 50Hz.
    emu_sound_scale_enable( sound_scale_enabled );

    video_color_update_palette(cached_video_canvas);
}

// -------------------------------------------------------------------------------
static void
set_model_pal( emu_screen_t *sc )
{

    // This will cause a HARD reset
    resources_set_int("MachineVideoStandard", MACHINE_SYNC_PAL );

    screen_start_line = sc->_start_line;
    visible_lines     = sc->pixel_height;

    // Apply audio frequency scaling to restore the original frequency 
    // since it will be shifted up when emulating PAL @ 60Hz.
    emu_sound_scale_enable( sound_scale_enabled );

    video_color_update_palette(cached_video_canvas);
}

// -------------------------------------------------------------------------------
int
emu_set_model( machine_model_t model )
{
    emu_capabilities_t *cap = emu_capabilities();
    emu_screen_t *sc;

    int i = 0;
    while( (sc = &(cap->screens[i])) && sc->screen_number >= 0 ) {
        if( sc->model == model ) break;
        i++;
    }
    if( sc->screen_number < 0 ) return -1; // Model not found

    configured_model = sc->model;
   
    // Switch internal emulator model
    switch( model & Model_Video_Type_Mask & Model_Strip_Modifier_Mask ) {
        case Model_Video_Type_NTSC: set_model_ntsc( sc ); break;
        case Model_Video_Type_PAL:  set_model_pal ( sc ); break;
        default: break;
    }

    v_adjust = 0;
    is_model_configured = 1;

    return 0;
}
// -------------------------------------------------------------------------------
//
int
emu_get_model()
{
    int model;

    if( !is_model_configured ) return -1; // If model never configured, say so!

    return configured_model;

    //resources_get_int("MachineVideoStandard", &model );
    //return model == MACHINE_SYNC_PAL ? Model_C64_PAL : Model_C64_NTSC;
}

// -------------------------------------------------------------------------------
//
void emu_key_capslock( int state )
{
    keyboard_shiftlock = state ? 1 : 0;
    // Immediately change the matrix state, otherwise shift will not be released
    // until AFTER the next key release
    keyboard_set_keyarr_any(1, 7, keyboard_shiftlock );
}

// -------------------------------------------------------------------------------
video_canvas_t *
video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped) 
{
    canvas->depth = DEPTH;
    canvas->width = 320; // NOT USED - TODO Remove from canvas struct
    canvas->height =240; // NOT USED - TODO Remove from canvas struct

    draw_buffer_t *db = canvas->draw_buffer;
    db->canvas_physical_width  = db->visible_width;
    db->canvas_physical_height = 240; //db->visible_height;

    // This probably has no effect, since we're using our own render/transfer functions
    canvas->videoconfig->rendermode = VIDEO_RENDER_RGB_1X1;
    canvas->videoconfig->double_size_enabled = 0;
    canvas->videoconfig->doublesizex = 0;
    canvas->videoconfig->doublesizey = 0;
    canvas->videoconfig->doublescan = 0;

    video_viewport_resize(canvas, 0);

    video_canvas_set_palette(canvas, canvas->palette);
    video_render_initraw(canvas->videoconfig);

    return canvas;
}

void
emu_set_vertical_shift( int adjust )
{
    int dlimit = (configured_model & Model_Video_Type_PAL) == Model_Video_Type_PAL ? -15 : -15; // Down shift
    int ulimit = (configured_model & Model_Video_Type_PAL) == Model_Video_Type_PAL ? +17 : +17; // Up shift

    if( adjust < dlimit ) adjust = dlimit;
    if( adjust > ulimit ) adjust = ulimit;
    
    v_adjust = adjust;

    // Now tha v_adjust is used directly in c64_canvas_to_rgba, this may not be needed.
    video_viewport_resize( cached_video_canvas, 0 );
}

void video_arch_canvas_init(struct video_canvas_s *canvas)
{
    canvas->video_draw_buffer_callback = NULL;

    cached_video_canvas = canvas;
}

// ------------------------------------------------------------------------------------------
#define BLIT_8_to_RGBA(src,dst) \
    dst[0] = colortab[src[0]]; \
    dst[1] = colortab[src[1]]; \
    dst[2] = colortab[src[2]]; \
    dst[3] = colortab[src[3]]; \
    dst[4] = colortab[src[4]]; \
    dst[5] = colortab[src[5]]; \
    dst[6] = colortab[src[6]]; \
    dst[7] = colortab[src[7]]; \
    src += 8; \
    dst += 8;

// ------------------------------------------------------------------------------------------
// Blit source rectangle 384 x 240 x  8bpp with origin (top-left) at 104, 31 + v_adjust, into
//      destination buf  384 x 240 x 32bpp with origin            at   0,  0
//      
__attribute__((optimize("unroll-loops")))
void
emu_transfer_canvas( void * reserved, unsigned char * dst_buffer, int xo, int yo, unsigned char * d, int len, void *priv )
{
    if (!cached_video_canvas->videoconfig->color_tables.updated) { /* update colors as necessary */
        // This only happens once.
        video_color_update_palette(cached_video_canvas);
    }

    const video_render_color_tables_t *color_tab = &(cached_video_canvas->videoconfig->color_tables);

    const DWORD       *colortab = color_tab->physical_colors;
    const unsigned int pitchs   = cached_video_canvas->draw_buffer->draw_buffer_width;
    const unsigned int pitcht   = cached_video_canvas->draw_buffer->canvas_physical_width * (DEPTH / 8);

    int sl = screen_start_line + ((screen_start_line == NORMAL_START_LINE) ? v_adjust : 0);

    BYTE *src = cached_video_canvas->draw_buffer->draw_buffer + pitchs * sl + 104;
    BYTE *trg = dst_buffer;

//printf("src: %p\ndst: %p\n", src, trg );
//printf("DWidth: %d\nPWidth: %d\n", cached_video_canvas->draw_buffer->draw_buffer_width, cached_video_canvas->draw_buffer->canvas_physical_width );
//printf("DHieght: %d\nPHeight: %d\n", cached_video_canvas->draw_buffer->draw_buffer_height, cached_video_canvas->draw_buffer->canvas_physical_height );
    
    // This is an ALIGNED transfer
    const BYTE *tmpsrc;
    DWORD *tmptrg;
    unsigned int y;

#pragma GCC unroll 120
#pragma GCC ivdep
    for (y = 0; y < PAL_FULL_VISIBLE_HEIGHT; y++) { // Full height is the same for PAL and NTSC
        tmpsrc = src;
        tmptrg = (DWORD *)trg;

        // 48 * 8 pixels = 384
        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 0
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);

        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 8
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);

        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 16
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);

        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 24
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);

        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 32
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);

        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 40
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);
        BLIT_8_to_RGBA(tmpsrc,tmptrg);

        src += pitchs;
        trg += pitcht;

        if(y == visible_lines - 1) break;
    }
}

// ----------------------------------------------------------------------------------

#define get_filepath(p) p

// ----------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------
// Returns 1 if file exists, 0 otherwise
static int
test_for_file( char *filepath )
{
    struct  stat sb;
    if( stat( filepath, &sb ) < 0 ) {
        //printf("Failed to find file %s\n", filepath );
        return 0;
    }

    if( !S_ISREG(sb.st_mode) ) {
        printf("File %s is not a regular file\n", filepath );
        return 0;
    }

    return 1;
}

// ----------------------------------------------------------------------------------
// dst must point to a char array long enough for the name
static void
create_disk_image_filename( char *dst, char const *fname )
{
    int l = strlen(USB_MOUNT_POINT);
    memcpy( dst, USB_MOUNT_POINT, l );
    dst[l] = '/';
    strcpy( dst + l + 1, fname );
}

// ----------------------------------------------------------------------------------
//
static int
attach_usb_disk_image( char *fpath )
{
    int  ret = 0;

    do {
        if( test_for_file( fpath ) == 0 ) break;
    	if( file_system_attach_disk( 8, fpath ) < 0 ) break;

        ret = 1;
    } while(0);

    return ret;
}

// -------------------------------------------------------------------------------
//
int xemu_insert_disk( const char *filename ) {
    return file_system_attach_disk( 8, filename );
}

// -------------------------------------------------------------------------------
//
int xemu_eject_disk( int device_id ) { // Can be -ve to indicate eject all
    file_system_detach_disk( device_id < 0 ? -1 : 8);
}

// -------------------------------------------------------------------------------
// Assumes storage has been mounted
// Returns 1 for internal (RO)
//         0 for external (RW)
int
emu_attach_default_storage( int has_external_storage, char const **fname )
{
    char fpath[256];

    if( has_external_storage ) {

        *fname = BASIC_DISK_IMAGE_NAME;
        create_disk_image_filename( fpath, *fname );

        // Look for, and attach a THEC64-drive8.d64 image (if one exists)
        if( attach_usb_disk_image( fpath ) ) return 0;

        // Instead create a disk image

        if( vdrive_internal_create_format_disk_image( 
                    fpath, "THEC64,01", DISK_IMAGE_TYPE_D64 ) == 0 ) {
            sync();

            // Now that has been created, attach it
            if( attach_usb_disk_image( fpath ) ) return 0;
        }

        // Failed to create default storage on external device....
    }

    // Mount a readonly disc (it is compressed)
    *fname = "";
    file_system_attach_disk( 8, get_filepath("/usr/share/the64/ui/data/blank.d64.gz") );

    return 1;
}

// -------------------------------------------------------------------------------
//
void core_cartridge_attach_image( const char * filename )
{
    cartridge_attach_image( CARTRIDGE_CRT, filename ); 
}

// -------------------------------------------------------------------------------
//
int  c64ui_init(){ return 0; }
void c64ui_shutdown() {}
int  c64scui_init(){ return 0; }
void c64scui_shutdown() {}

#define PNS PAL_NORMAL_START_LINE
#define PNH PAL_NORMAL_VISIBLE_HEIGHT
#define PFS PAL_FULL_START_LINE
#define PFH PAL_FULL_VISIBLE_HEIGHT

#define NNS NTSC_NORMAL_START_LINE
#define NNH NTSC_NORMAL_VISIBLE_HEIGHT
#define NFS NTSC_FULL_START_LINE
#define NFH NTSC_FULL_VISIBLE_HEIGHT

static emu_capabilities_t capabilities = {
    Model_C64,
    "C64",
    {  //                   sl, sw  sh  sd  pox poy pw   ph   Display Mode widths  Virtual Keyboard shifts
      { 0, Model_C64_PAL,   PNS,384,PNH,32, 32, 35, 320, 200, { 1152, 1078, 864 }, { -88, -60, 0 } },
      { 0, Model_C64_NTSC,  NNS,384,NNH,32, 32, 23, 320, 200, { 1152, 1078, 864 }, { -88, -60, 0 } },
      { 0, Model_C64_PALF,  PFS,384,PFH,32, 32, 35, 320, 200, { 1152, 1078, 864 }, { -88, -60, 0 } },
      { 0, Model_C64_NTSCF, NFS,384,NFH,32, 32, 23, 320, 200, { 1152, 1078, 864 }, { -88, -60, 0 } },
      { -1 }
    },
    1,
    0
};

EMU_EXPORT_ID(capabilities);

emu_capabilities_t * emu_capabilities()
{
    return &capabilities;
}

// -------------------------------------------------------------------------------
