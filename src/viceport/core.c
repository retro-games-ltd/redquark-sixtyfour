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
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>

#include "videoarch.h"
#include "machine.h"
#include "sem.h"
#include <stdlib.h>
#include <sys/time.h>

#include "emucore.h"

// Vice includes
#include "c64/c64model.h"
#include "c64/c64.h"
#include "sid/sid.h"
#include "archapi.h"
#include "maincpu.h"
#include "drive/drive.h"
#include "sysfile.h"
#include "gfxoutput.h"
#include "init.h"
#include "resources.h"
#include "log.h"
#include "initcmdline.h"
#include "video.h"
#include "sound.h"
#include "uitraps.h"

int console_mode = 0;
int video_disabled_mode = 0;
static int initialised = 0;
static int started = 0;
static int needs_shutdown = 0;

int main_program(int argc, char **argv);
static void core_exit(void);

static pthread_t emuthread;
extern int maincpu_running;

// Provided by each emulation core
extern const char const *emu_id;
void core_init();
void core_start();

// -------------------------------------------------------------------------------
//
static int
_emu_init(int argc, char **argv)
{
    if( semaphore_init() < 0 ) return -1;

    archdep_init(&argc, argv);

    if (atexit(core_exit) < 0) return -1;

    maincpu_early_init();
    machine_setup_context();
    drive_setup_context();
    machine_early_init();

    sysfile_init(machine_name); // Initialize system file locator
    
    gfxoutput_early_init();

    if (init_resources() < 0 ) return -1;
    
    /* Set factory defaults.  */
    if (resources_set_defaults() < 0) return -1;

    /* Load the user's default configuration file.  */
    if (resources_load(NULL) < 0) {
        /* The resource file might contain errors, and thus certain
           resources might have been initialized anyway.  */
        if (resources_set_defaults() < 0) {
            printf("Cannot set defaults.\n");
            return -1;
        }
    }

    if (video_init() < 0) return -1;

    core_init(); // Provided by specific core

    resources_set_int("SoundVolume", 100 );
    resources_set_int("SoundOutput", 2); 
    resources_set_int("SoundBufferSize", 1000); // ms -> comes out at 185ms?

    if (init_main() < 0) return -1;

    return 1;
}

// -------------------------------------------------------------------------------
// This does not discard the emulator - it is still there, but it's execution thread
// is terminated.
static void _end_emulator_thread()
{
    // FIXME This sometimes causes a SEGV - Find out how to gracefully stop VICE
    // FIXME When the SEGV gets raised, core_exit() will get called, which we don't
    //       want to happen when stopping the thread.
    int e;
    if( (e = pthread_cancel( emuthread )) < 0 ) {
        printf("pthread_cancel failed with err %d\n", e );
    }
}

// -------------------------------------------------------------------------------
//
static void core_exit(void)
{
    if( needs_shutdown ) machine_shutdown();
    _end_emulator_thread();
}

// -------------------------------------------------------------------------------
//
static void *
emulator_loop( void *p )
{
    int ot;
    pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, &ot );

    needs_shutdown = 1; // Make sure core_atexit() shuts down the emulator internals

    emu_core_reset( Emu_Reset_Hard );

    maincpu_running = 1;

    maincpu_mainloop();

    sound_close(); // Make sure any open ALSA connection is released
    started = 0;

    return 0;
}

// -------------------------------------------------------------------------------
//
void
emu_stop()
{
    if( !started ) return;

    if( schedule_emulator_quit() < 0 ) {
        printf("ERROR - Quit requested but core was not paused!\n"); // Should never happen as pause should be forced..
    }
    pthread_join( emuthread, NULL );
}

// -------------------------------------------------------------------------------
//
int
emu_start( )
{
    if( !initialised ) return -1;
    if(  started     ) return -2;


    int iret = pthread_create( &emuthread, NULL, emulator_loop, (void*)NULL);
    if(iret)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        return -1;
    }

    // Wait for at least one frame to complete, then we know that we've started!
    // To make sure we get at least one frame, just in case the core's internal
    // frame is further ahead than the start, we cycle an additional frame also.
    // This is just for safety, as the core frame loop should have quit at the
    // start of the frame (see emu_stop).
    semaphore_wait_for( SEM_FRAME_DONE );

    semaphore_notify( SEM_START_NEXT_FRAME );
    semaphore_wait_for( SEM_FRAME_DONE );

    // Run the core-specific startup code once we're sure the machine is running
    core_start();

    started = 1;

    return 0;
}

// -------------------------------------------------------------------------------
//
int
emu_is_running()
{
    return started;
}

// -------------------------------------------------------------------------------
//
int
emu_initialise()
{
    static char *av = "./the64";

    if( initialised == 0 ) initialised = _emu_init(1, &av);

    return initialised;
}

// -------------------------------------------------------------------------------
