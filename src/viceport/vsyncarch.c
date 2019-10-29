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

#include "vsyncapi.h"
#include "videoarch.h"
#include "sem.h"

#include "kbdbuf.h"
#include <time.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define THROTTLE 50

#define TICKS_PER_SECOND  1000000000L  /* Nanoseconds resolution. */
#define TICKS_PER_MSEC    1000000L
#define TICKS_PER_USEC    1000L

/* ------------------------------------------------------------------------- */

/* Number of timer units per second. */
signed long vsyncarch_frequency(void)
{
    /* Milliseconds resolution. */
    return TICKS_PER_SECOND;
}

/* Get time in timer units. */
unsigned long vsyncarch_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    return (TICKS_PER_SECOND * tv.tv_sec) + (TICKS_PER_USEC * tv.tv_usec);
}

void vsyncarch_init(void)
{
}

/* Display speed (percentage) and frame rate (frames per second). */
void vsyncarch_display_speed(double speed, double frame_rate, int warp_enabled)
{
    printf( "Speed: %f  Rate: %f   Warp: %d\n", (float)speed, (float)frame_rate, warp_enabled);
}

/* Sleep a number of timer units. */
void vsyncarch_sleep(signed long delay)
{
    /* HACK: to prevent any multitasking stuff getting in the way, we return
     *              immediately on delays up to 0.1ms */
    if (delay < (TICKS_PER_MSEC / 10)) {
        return;
    }

    struct timespec ts;
    ts.tv_sec = delay / TICKS_PER_SECOND;
    ts.tv_nsec = (delay % TICKS_PER_SECOND);

    while (nanosleep(&ts, &ts));
}

int kpress = 0;

void vsyncarch_presync(void)
{
    // Let the main thread start updating the display, while vsync.c continues to 
    // update the sound.
    semaphore_notify( SEM_FRAME_DONE );
}


extern void _ui_action_pause();
extern void virtual_keyboard_process_keys();
extern void joystick_process_queue();

typedef void (*PostSyncCallback)();

static PostSyncCallback post_sync_cb = NULL;

void emu_sync_callback( PostSyncCallback pscb ) {
    post_sync_cb = pscb;
}


void
vsyncarch_postsync(void)
{
//printf("POST\n");
    _ui_action_pause();

    kbdbuf_flush();

    if( post_sync_cb ) post_sync_cb();
    //joystick_process_queue();
    //virtual_keyboard_process_keys();
}

// When vsyncarch_sync_with_raster() is called, we know the emu thread has completed one frame
void
vsyncarch_sync_with_raster(video_canvas_t *c)
{
    static long now = 0;
    static long then = 0;
    static long flen = 0;
    static  int oth = 0;

#ifdef TIME_DEBUG
    now = vsyncarch_gettime();
    if( then > 0 ) {
        flen += (now - then);
#if THROTTLE
        if( oth % THROTTLE == 0 ) {
            printf( "...c64 signal frame done\n");
            flen /= THROTTLE;
#endif
            printf( "   c64 frame ran for: %f us\n", (float)flen / 1000 );
            flen = 0;
#if THROTTLE
        }
#endif
    }

#if THROTTLE
    if( oth % THROTTLE == 0 ) printf( "...c64 wait for next frame\n");
    oth++;
#endif
#endif // TIME_DEBUG

    //fprintf(stderr, "Wait for next frame start\n");
    semaphore_wait_for( SEM_START_NEXT_FRAME );
    //fprintf(stderr, "Starting next frame\n");
    
    then = vsyncarch_gettime();
}

int vsyncarch_vbl_sync_enabled(void)
{
    return 1;
}   
