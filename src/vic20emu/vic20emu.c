/*
 *  THEC64 Mini - Vic20 core
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
#include "videoarch.h"
#include "video.h"
#include "palette.h"
#include "keyboard.h"
#include "viewport.h"
#include "video/render1x1.h"

#include "vdrive/vdrive-internal.h"
#include "attach.h"
#include "diskimage.h"
#include "cartridge.h"
#include "resources.h"
#include "c64/c64model.h"
#include "vic20/vic20.h"
#include "sid/sid.h"
#include "sound.h"
#include "emucore.h"
#include "../ui/usb.h"
#include "../ui/emu_bind_decl.h"
#include "../ui/machine_model.h"

#ifndef GLOBAL_OUTPUT
#   ifdef PAL_OUTPUT
#      warning PAL Output
#   else
#      warning NTSC Output
#   endif
#endif

#define BASIC_DISK_IMAGE_NAME "THEC64-drive8.d64"

#define PAL_NORMAL_START_LINE        48
#define PAL_NORMAL_VISIBLE_HEIGHT   240
#define NTSC_NORMAL_START_LINE       21
#define NTSC_NORMAL_VISIBLE_HEIGHT  240

#define PAL_FULL_START_LINE          29
#define PAL_FULL_VISIBLE_HEIGHT     283
#define NTSC_FULL_START_LINE          8
#define NTSC_FULL_VISIBLE_HEIGHT    253

#define PAL_LINES            VIC20_PAL_SCREEN_LINES        // 312
#define PAL_LINE_CLKS        VIC20_PAL_CYCLES_PER_LINE     // 71
#define PAL_FRAME_CLKS       (PAL_LINES * PAL_LINE_CLKS)   // 312 * 71
#define PAL_CLKS_SECOND_TRUE (PAL_FRAME_CLKS * 50)

#define NTSC_LINES            VIC20_NTSC_SCREEN_LINES         // 261
#define NTSC_LINE_CLKS        VIC20_NTSC_CYCLES_PER_LINE      // 65
#define NTSC_FRAME_CLKS       (NTSC_LINES * NTSC_LINE_CLKS)   // 261 * 65
#define NTSC_CLKS_SECOND_TRUE (NTSC_FRAME_CLKS * 60)

static video_canvas_t *cached_video_canvas = NULL;
static int v_adjust = 0;  // +ve means move up. -ve down  ( -16 <= adjust <= 16 )

static int  pal_screen_start_line  = PAL_NORMAL_START_LINE;
static int  pal_visible_lines      = PAL_NORMAL_VISIBLE_HEIGHT;
static int  ntsc_screen_start_line = NTSC_NORMAL_START_LINE;
static int  ntsc_visible_lines     = NTSC_NORMAL_VISIBLE_HEIGHT;

static long pal_clks_second  = PAL_CLKS_SECOND_TRUE;
static long ntsc_clks_second = NTSC_CLKS_SECOND_TRUE;

static int  video_output_60  = 1;

static void _configure_video_50hz();
static void _configure_video_60hz();

static int             is_model_configured = 0;
static machine_model_t configured_model    = 0;

static void set_transfer_function( int model );

static void (*transfer_function)( unsigned char * ) = NULL;

emu_capabilities_t * emu_capabilities();

// -------------------------------------------------------------------------------
// Core specific initialisation
int
core_init()
{
    // Turning sound warp mode on prevents start-up clicks when Vice starts up
    // its sound core.
    sound_set_warp_mode(1);

    // Prevent VICE from using the raw filesystem as a Disk FIXME Valid for Vic20?
    resources_set_int("FileSystemDevice8",  0 );
    resources_set_int("FileSystemDevice9",  0 );
    resources_set_int("FileSystemDevice10", 0 );
    resources_set_int("FileSystemDevice11", 0 );
    resources_set_int("AutostartPrgMode",   1 ); // Handle PRG as injection
    //resources_set_int("AutostartPrgMode",   2 ); // Handle PRG's as though they on their own disk image (AUTOSTART_PRG_MODE_DISK)
    //resources_set_string("AutostartPrgDiskImage", "/tmp/prgdisk.d64" ); // Blank disk to create/use to hadle PRG loading

    resources_set_int("AutostartDelay", 3 ); // Use the default of 3

    resources_set_int("RAMInitStartValue", 0 );

    resources_set_int("VICDoubleSize", 0);
    resources_set_int("VICExternalPalette", 0);

    resources_set_int("Sound", 1);

    //resources_set_string("VICPaletteFile", "theC64-palette.vpl" );
    //resources_set_int   ("VICExternalPalette", 1);

    int vice_model = MACHINE_SYNC_PAL;
    resources_set_int("MachineVideoStandard", vice_model );
    set_transfer_function( vice_model );

    is_model_configured = 0;
    configured_model = Model_Video_Type_PAL;
}

// -------------------------------------------------------------------------------
// Core specific startup
int
core_start()
{
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
    // When running at 60Hz, we need to generate 735 (44100/60) samples per frame.
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

    // When TheC64 runs the display at 50Hz, we have to do some magic to get 
    // the SID generating 882 samples (44100/50) per frame.
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
}

// -------------------------------------------------------------------------------
static void
set_model_ntsc( emu_screen_t *sc )
{

    // Internal model has changed, flip vice over
    resources_set_int("MachineVideoStandard", MACHINE_SYNC_NTSC );
    set_transfer_function( MACHINE_SYNC_NTSC );

    ntsc_screen_start_line = sc->_start_line;
    ntsc_visible_lines     = sc->pixel_height;

    // Apply audio frequency scaling to restore the original frequency 
    // since it will be shifted down when emulating NTSC @ 50Hz.
    float d = ntsc_clks_second - NTSC_CLKS_SECOND_TRUE;
    float freq_scale = 1.0 - d / (d < 0 ? ntsc_clks_second : NTSC_CLKS_SECOND_TRUE);
    sound_set_audio_scaling( freq_scale ); // TODO for Vic20
}

// -------------------------------------------------------------------------------
static void
set_model_pal( emu_screen_t *sc )
{

    resources_set_int("MachineVideoStandard", MACHINE_SYNC_PAL );
    set_transfer_function( MACHINE_SYNC_PAL );

    pal_screen_start_line = sc->_start_line;
    pal_visible_lines     = sc->pixel_height;

    // Apply audio frequency scaling to restore the original frequency 
    // since it will be shifted up when emulating PAL @ 60Hz.
    float d = pal_clks_second - PAL_CLKS_SECOND_TRUE;
    float freq_scale = 1.0 - d / (d < 0 ? pal_clks_second : PAL_CLKS_SECOND_TRUE);
    sound_set_audio_scaling( freq_scale ); // TODO for Vic20
}

// -------------------------------------------------------------------------------
int
emu_set_model( machine_model_t model )
{
    emu_capabilities_t *cap = emu_capabilities();
    emu_screen_t *sc = NULL;

    int i = 0;
    while( (sc = &(cap->screens[i])) && sc->screen_number >= 0 ) {
        if( sc->model == model ) break;
        i++;
    }
    if( sc->screen_number < 0 ) return -1; // Model not found
    // printf("Found screen capability id %d\n", i );

    // Switch internal emulator model
    switch( model & Model_Video_Type_Mask & Model_Strip_Modifier_Mask ) {
        case Model_Video_Type_NTSC: set_model_ntsc( sc ); break;
        case Model_Video_Type_PAL:  set_model_pal ( sc ); break;
        default: break;
    }

    v_adjust = 0;
    configured_model = sc->model;
    is_model_configured = 1;

    return 0;
}

// -------------------------------------------------------------------------------
machine_model_t
emu_get_model()
{
    int model;

    if( !is_model_configured ) return -1; // If model never configured, say so!

    return configured_model;

    //resources_get_int("MachineVideoStandard", &model );
    //return model == MACHINE_SYNC_PAL ? Model_VIC20_PAL : Model_VIC20_NTSC;
}

// -------------------------------------------------------------------------------
//
void emu_key_capslock( int state )
{
    keyboard_shiftlock = state ? 1 : 0;
    // Immediately change the matrix state, otherwise shift will not be released
    // until AFTER the next key release
    keyboard_set_keyarr_any(1, 3, keyboard_shiftlock );
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
// render_32_1x1_04 width  224  height 284  xs,ys: 552, 28  xt,yt:   0,  0  pitchs: 1328 pitcht:  896   <- PAL
// ------------------------------------------------------------------------------------------
// Blit source rectangle 224 x 284 x  8bpp with origin (top-left) at 552, 48 + v_adjust, into
//      destination buf  224 x 240 x 32bpp with origin            at   0,        
__attribute__((optimize("unroll-loops")))
static void
_transfer_canvas_pal( unsigned char * dst_buffer )
{
    if (!cached_video_canvas->videoconfig->color_tables.updated) { /* update colors as necessary */
        // This only happens once.
        video_color_update_palette(cached_video_canvas);
    }

    const video_render_color_tables_t *color_tab = &(cached_video_canvas->videoconfig->color_tables);
    const DWORD       *colortab = color_tab->physical_colors;
    const unsigned int pitchs   = cached_video_canvas->draw_buffer->draw_buffer_width;
    const unsigned int pitcht   = cached_video_canvas->draw_buffer->canvas_physical_width * (DEPTH / 8);

    // Do not apply vertical adjust if fullscree is enabled
    int sl = pal_screen_start_line + ((pal_screen_start_line == PAL_NORMAL_START_LINE) ? v_adjust : 0);

    BYTE *src = cached_video_canvas->draw_buffer->draw_buffer + pitchs * sl + 552;
    BYTE *trg = dst_buffer;

//printf("_transfer_canvas_pal width %4d  height %3d  pitchs: %4d pitcht: %4d\n", cached_video_canvas->draw_buffer->canvas_physical_width, cached_video_canvas->draw_buffer->canvas_physical_height, pitchs, pitcht );

    // This is an ALIGNED transfer
    const BYTE *tmpsrc;
    DWORD *tmptrg;
    unsigned int y;

#pragma GCC unroll 120
#pragma GCC ivdep
    for (y = 0; y < PAL_FULL_VISIBLE_HEIGHT; y++) {
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
        BLIT_8_to_RGBA(tmpsrc,tmptrg); // <- 224 wide PAL

        src += pitchs;
        trg += pitcht;

        if(y == pal_visible_lines - 1) break;
    }
}

// ------------------------------------------------------------------------------------------
// render_32_1x1_04 width  200  height 253  xs,ys: 464,  8  xt,yt:   0,  0  pitchs: 1120 pitcht:  800   <- NTSC
// ------------------------------------------------------------------------------------------
// Blit source rectangle 200 x 253 x  8bpp with origin (top-left) at 464, 21 + v_adjust, into
//      destination buf  200 x 240 x 32bpp with origin            at   0,  0
//      
__attribute__((optimize("unroll-loops")))
static void
_transfer_canvas_ntsc( unsigned char * dst_buffer )
{
    if (!cached_video_canvas->videoconfig->color_tables.updated) { /* update colors as necessary */
        // This only happens once.
        video_color_update_palette(cached_video_canvas);
    }

    const video_render_color_tables_t *color_tab = &(cached_video_canvas->videoconfig->color_tables);
    const DWORD       *colortab = color_tab->physical_colors;
    const unsigned int pitchs   = cached_video_canvas->draw_buffer->draw_buffer_width;
    const unsigned int pitcht   = cached_video_canvas->draw_buffer->canvas_physical_width * (DEPTH / 8);

    // Do not apply vertical adjust if fullscree is enabled
    int sl = ntsc_screen_start_line + ((ntsc_screen_start_line == NTSC_NORMAL_START_LINE) ? v_adjust : 0);

    //BYTE *src = cached_video_canvas->draw_buffer->draw_buffer + pitchs * 21 + (464 + 4 * 12); // 464 FOR LEFT-SHIFT-PATCHED-VICE
    BYTE *src = cached_video_canvas->draw_buffer->draw_buffer + pitchs * sl + 464;
    BYTE *trg = dst_buffer;

//printf("_transfer_canvas_ntsc width %4d  height %3d  pitchs: %4d pitcht: %4d\n", cached_video_canvas->draw_buffer->canvas_physical_width, cached_video_canvas->draw_buffer->canvas_physical_height, pitchs, pitcht );

    // This is an ALIGNED transfer
    const BYTE *tmpsrc;
    DWORD *tmptrg;
    unsigned int y;

#pragma GCC unroll 120
#pragma GCC ivdep
    for (y = 0; y < NTSC_FULL_VISIBLE_HEIGHT; y++) {
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

        BLIT_8_to_RGBA(tmpsrc,tmptrg); // 24 <-- 200 wide NTSC

        src += pitchs;
        trg += pitcht;
    
        if(y == ntsc_visible_lines - 1) break;
    }
}

// -------------------------------------------------------------------------------
//
static void
set_transfer_function( int model )
{
    if( model == MACHINE_SYNC_PAL )  transfer_function = _transfer_canvas_pal;
    else                             transfer_function = _transfer_canvas_ntsc;
}

// -------------------------------------------------------------------------------
//
void
emu_transfer_canvas( void * reserved, unsigned char * dst_buffer, int xo, int yo, unsigned char * d, int len, void *priv )
{
    if( transfer_function ) transfer_function( dst_buffer );
}

// -------------------------------------------------------------------------------
//
video_canvas_t *
video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
    canvas->depth = DEPTH;

    draw_buffer_t *db = canvas->draw_buffer;
    db->canvas_physical_width  = db->visible_width;
    db->canvas_physical_height = 240;

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

// -------------------------------------------------------------------------------
//
void video_arch_canvas_init(struct video_canvas_s *canvas)
{
    canvas->video_draw_buffer_callback = NULL;

    cached_video_canvas = canvas;
}

// -------------------------------------------------------------------------------
//
void emu_set_vertical_shift( int adjust )
{
    int dlimit = (configured_model & Model_Video_Type_PAL) == Model_Video_Type_PAL ? -16 : -13; // Down shift
    int ulimit = (configured_model & Model_Video_Type_PAL) == Model_Video_Type_PAL ? +16 : +0;  // Up shift

    if( adjust < dlimit ) adjust = dlimit;
    if( adjust > ulimit ) adjust = ulimit;
    
    v_adjust = adjust;

    // Now tha v_adjust is used directly in c64_canvas_to_rgba, this may not be needed.
    video_viewport_resize( cached_video_canvas, 0 );
}

// -------------------------------------------------------------------------------
void * vicii_get_canvas() { return cached_video_canvas; } // Called from emu_screenshot_save() 

// -------------------------------------------------------------------------------
//
int emu_insert_disk( const char *filename ) {
    return file_system_attach_disk( 8, filename );
}

// -------------------------------------------------------------------------------
//
int emu_eject_disk( int device_id ) { // Can be -ve to indicate eject all
    file_system_detach_disk( device_id < 0 ? -1 : 8);
}

// ----------------------------------------------------------------------------

#define get_filepath(p) p

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
// -------------------------------------------------------------------------------
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
// Assumes storage has been mounted
// Returns 1 for internal (RO)
//         0 for external (RW)
int
emu_attach_default_storage( int has_external_storage, const char **fname  )
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
// Expects a .cr[1245ABMFPT] extension
//
void core_cartridge_attach_image( const char * filename )
{
    int type = CARTRIDGE_VIC20_GENERIC;
    char t   = filename[ strlen(filename) - 1 ];

    if(t >= 'a' && t <= 'z' ) t -= ('a' - 'A'); // Make Upper case

    switch( t ) {
        case 'T' : type = CARTRIDGE_VIC20_GENERIC;   break;
        case '2' : type = CARTRIDGE_VIC20_16KB_2000; break;
        case '4' : type = CARTRIDGE_VIC20_16KB_4000; break;
        case '6' : type = CARTRIDGE_VIC20_16KB_6000; break;
        case 'A' : type = CARTRIDGE_VIC20_8KB_A000;  break;
        case 'B' : type = CARTRIDGE_VIC20_4KB_B000;  break;
        case 'M' : type = CARTRIDGE_VIC20_MEGACART;  break;
        case 'F' : type = CARTRIDGE_VIC20_FINAL_EXPANSION;  break;
        case 'P' : type = CARTRIDGE_VIC20_FP;  break; // Flash Pllugin
    }

    cartridge_attach_image( type, filename ); 
}

// -------------------------------------------------------------------------------
//
void core_cartridge_trigger_freeze()
{
}

// -------------------------------------------------------------------------------
//
int  vic20ui_init(){ return 0; }
void vic20ui_shutdown() {}

// PAL  = 284(w) x xxx(h)    224 x 283
// NTSC = 260(w) x XXX(h)    200 x 233
//
// Using http://sleepingelephant.com/ipw-web/bulletin/bb/viewtopic.php?t=4413 as a reference
// Using normal height of 284 (there is a full height of 294 though...)

// Vic20 pixels are twice as wide as they are high, hence *2
#define P_PP_W 1220 // Calculated pal width = 1344, which is too wide. We need then L+R black borders for VK, so set to 1280 - 50
#define P_EU_W 1120 // (224 * 2) * 0.833 * 3 = 1120  | or ((224*2) * 720)/240 * 0.833 
#define P_US_W  903 // (200 * 2) * 0.752 * 3 = 902.4 | or ((200*2) * 720)/240 * 0.752

#define N_PP_W 1200 // Calculated pal width = 1200 ... less than the PAL 1344
#define N_EU_W 1120 // (224 * 2) * 0.833 * 3 = 1120  | or ((224*2) * 720)/240 * 0.833 
#define N_US_W  903 // (200 * 2) * 0.752 * 3 = 902.4 | or ((200*2) * 720)/240 * 0.752

#define PNS PAL_NORMAL_START_LINE
#define PNH PAL_NORMAL_VISIBLE_HEIGHT
#define PFS PAL_FULL_START_LINE
#define PFH PAL_FULL_VISIBLE_HEIGHT

#define NNS NTSC_NORMAL_START_LINE
#define NNH NTSC_NORMAL_VISIBLE_HEIGHT
#define NFS NTSC_FULL_START_LINE
#define NFH NTSC_FULL_VISIBLE_HEIGHT

static emu_capabilities_t capabilities = {
    Model_VIC20,
    "VIC20",
    {
       //                     sl  sw  sh  sd  pox poy pw   ph   Display Mode widths         Virtual Keyboard shifts
      { 0, Model_VIC20_PAL,   PNS,224,PNH,32, 48, 48, 176, 184, { P_PP_W, P_EU_W, P_US_W }, { -130, -75, 0 } },
      { 0, Model_VIC20_NTSC,  NNS,200,NNH,32, 20, 42, 176, 184, { N_PP_W, N_EU_W, N_US_W }, { -90,  -75, 0 } },
      { 0, Model_VIC20_PALF,  PFS,224,PFH,32, 48, 48, 176, 184, { P_PP_W, P_EU_W, P_US_W }, { -130, -75, 0 } },
      { 0, Model_VIC20_NTSCF, NFS,200,NFH,32, 20, 42, 176, 184, { N_PP_W, N_EU_W, N_US_W }, { -90,  -75, 0 } },
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
