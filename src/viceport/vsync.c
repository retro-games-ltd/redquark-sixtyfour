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
/*
 * vsync.c - End-of-frame handling
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
 *  Dag Lem <resid@nimrod.no>
 *  Thomas Bretz <tbretz@gsi.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

/* This does what has to be done at the end of each screen frame (50 times per
   second on PAL machines). */

/* NB! The timing code depends on two's complement arithmetic.
   unsigned long is used for the timer variables, and the difference
   between two time points a and b is calculated with (signed long)(b - a)
   This allows timer variables to overflow without any explicit
   overflow handling.
*/

int vsync_frame_counter;

#include "vice.h"

/* Port me... */
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "clkguard.h"
#include "cmdline.h"
#include "debug.h"
#include "log.h"
#include "maincpu.h"
#include "machine.h"
#ifdef HAVE_NETWORK
#include "monitor_network.h"
#endif
#include "network.h"
#include "resources.h"
#include "sound.h"
#include "translate.h"
#include "types.h"
#if (defined(WIN32) || defined (HAVE_OPENGL_SYNC)) && !defined(USE_SDLUI)
#include "videoarch.h"
#endif
#include "vsync.h"
#include "vsyncapi.h"

#include "emucore.h"
#include "sem.h"

#undef HAVE_OPENGL_SYNC

#define AUDIO_FRAME_DRIFT_ADJUST

/* ------------------------------------------------------------------------- */

/* Relative speed of the emulation (%).  0 means "don't limit speed". */
static int relative_speed;

/* Refresh rate.  0 means "auto". */
static int refresh_rate;

/* "Warp mode".  If nonzero, attempt to run as fast as possible. */
static int warp_mode_enabled;

/* ------------------------------------------------------------------------- */
static int set_relative_speed(int val, void *param)
{
    relative_speed = val;
    sound_set_relative_speed(relative_speed);

    return 0;
}

/* ------------------------------------------------------------------------- */
static int set_refresh_rate(int val, void *param)
{
    if (val < 0)
        return -1;

    refresh_rate = val;

    return 0;
}

/* ------------------------------------------------------------------------- */
static int set_warp_mode(int val, void *param)
{
    warp_mode_enabled = val;
    sound_set_warp_mode(warp_mode_enabled);

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Vsync-related resources. */
static const resource_int_t resources_int[] = {
    { "Speed", 100, RES_EVENT_SAME, NULL,
      &relative_speed, set_relative_speed, NULL },
    { "RefreshRate", 0, RES_EVENT_STRICT, (resource_value_t)1,
      &refresh_rate, set_refresh_rate, NULL },
    { "WarpMode", 0, RES_EVENT_STRICT, (resource_value_t)0,
      /* FIXME: maybe RES_EVENT_NO */
      &warp_mode_enabled, set_warp_mode, NULL },
    { NULL }
};

int vsync_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

/* Vsync-related command-line options. */
static const cmdline_option_t cmdline_options[] = {
    { "-speed", SET_RESOURCE, 1,
      NULL, NULL, "Speed", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_PERCENT, IDCLS_LIMIT_SPEED_TO_VALUE,
      NULL, NULL },
    { "-refresh", SET_RESOURCE, 1,
      NULL, NULL, "RefreshRate", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_UPDATE_EVERY_VALUE_FRAMES,
      NULL, NULL },
    { "-warp", SET_RESOURCE, 0,
      NULL, NULL, "WarpMode", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_WARP_MODE,
      NULL, NULL },
    { "+warp", SET_RESOURCE, 0,
      NULL, NULL, "WarpMode", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_WARP_MODE,
      NULL, NULL },
    { NULL }
};

int vsync_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */
/* Number of frames per second on the real machine. */
static double refresh_frequency;

/* Function to call at the end of every screen frame. */
static void (*vsync_hook)(void);

/* ------------------------------------------------------------------------- */
/* static guarantees zero values. */

static void clk_overflow_callback(CLOCK amount, void *data)
{
}

/* ------------------------------------------------------------------------- */

void vsync_set_machine_parameter(double refresh_rate, long cycles)
{
    refresh_frequency = refresh_rate;
}

/* ------------------------------------------------------------------------- */
double vsync_get_refresh_frequency(void)
{
    return refresh_frequency;
}

/* ------------------------------------------------------------------------- */
void vsync_init(void (*hook)(void))
{
    vsync_hook = hook;
    vsync_suspend_speed_eval();
    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    vsyncarch_init();
}

/* ------------------------------------------------------------------------- */
/* FIXME: This function is not needed here anymore, however it is
   called from sound.c and can only be removed if all other ports are
   changed to use similar vsync code. */
int vsync_disable_timer(void)
{
    return 0;
}

/* ------------------------------------------------------------------------- */
/* This should be called whenever something that has nothing to do with the
   emulation happens, so that we don't display bogus speed values. */
void vsync_suspend_speed_eval(void)
{
    network_suspend();
    sound_suspend();
    vsync_sync_reset();
}

static int skipping = 0;
/* ------------------------------------------------------------------------- */
/* This resets sync calculation after a "too slow" or "sound buffer
   drained" case. */
void vsync_sync_reset(void)
{
    skipping = 0;
}

/* ------------------------------------------------------------------------- */
/* This is called at the end of each screen frame. It flushes the audio buffer
 * and stops it falling behind the the video (locked to HDMI vsync)
 */
int vsync_do_vsync(struct video_canvas_s *c, int been_skipped)
{
    /*
     * Selectively performing a FRAME_DONE causes a UI lockup (waiting for FDone)
     * in cases where warp-mode is left on (the case when autoloading from a
     * blank C64 .d64 file, resulting in FILE NOT FOUND and warp-mode remaining
     * enabled.
     *
#ifdef AUDIO_FRAME_DRIFT_ADJUST
    if( !skipping )
#endif
    */
        vsyncarch_presync(); // does a notify FRAME_DONE

    vsync_hook();

    /* Flush sound buffer, get delay in seconds. */
    double d = sound_flush(); // We always have to do this, even in warp mode (previously was disabled if warp_mode_enabled - see below).

    if (!warp_mode_enabled ) {
        vsyncarch_sync_with_raster(c); // does a wait_for START_NEXT_FRAME
        skipping = 0;
    } else {
        skipping = 1;
    }

#ifdef AUDIO_FRAME_DRIFT_ADJUST
    // Video/emulation is getting ahead of the audio playback.
    // Skip a frame to allow the audio to catch up a bit (20ms@50Hz 16ms@60Hz)
    // 5 frames at 50Hz / 6 frames at 60Hz is 0.1 seconds.
    if( !skipping && d <= -0.1 ) {
        semaphore_notify( SEM_FRAME_DONE );
        semaphore_wait_for( SEM_START_NEXT_FRAME );
    }
#endif

    vsyncarch_postsync(); // Process waiting pause request etc

    return 0;
}

/* ------------------------------------------------------------------------- */
