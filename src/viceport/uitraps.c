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

//#define TRDEBUG
#ifdef TRDEBUG
#   define printd(...) fprintf(stderr, __VA_ARGS__)
#else
#   define printd(...) {;} //(0)
#endif
static int paused  = 0;
static int loading = 0;
static char *snapshot_filename = NULL;

static void save_snapshot_trap( WORD unused_addr, void *data );
static void load_snapshot_trap( WORD unused_addr, void *data );

static void wait_for_stable_pause_state();

extern int vsyncarch_framecount;

static int pause_current_state = 0;
static int pause_target_state  = 0;

static int quit_requested = 0;
extern int maincpu_running;

// ----------------------------------------------------------------------------------
//
static void
pause_trap(WORD addr, void *data)
{
    pause_current_state = 1;

    do {
        printd("< Simulate normal running thread sync\n");

        semaphore_notify( SEM_FRAME_DONE );
        if(!pause_target_state) break;

        semaphore_wait_for( SEM_START_NEXT_FRAME );

        printd("< Pause frame done... test for quit, pause release or snapshot save/load....\n");

        if( quit_requested ) {
            maincpu_running    = 0; // Stop the cpu loop
            quit_requested     = 0; // Cancel the quit request
            pause_target_state = 0; // exit the paused state so that the cpu loop completes
            break;
        }

        // Save/Load may have been requested during a pause, which is a special case.
        if( !snapshot_filename ) continue;

        printd("< Save/Load snapshot from pause\n");

        if( loading ) {
            load_snapshot_trap(0, (void *)snapshot_filename);
        } else {
            save_snapshot_trap(0, (void *)snapshot_filename);
        }
        snapshot_filename = NULL;

        printd("< Pause notify caller that save has completed\n");
        semaphore_notify( SEM_SAVE_LOAD_DONE );

        // Loop and sit back in the paused state
    } while( pause_target_state );

    pause_current_state = 0;
}

// ----------------------------------------------------------------------------

// pause request is actioned at the end of a frame by vsyncarch_postsync()
void
_ui_action_pause()
{
    if( pause_target_state == pause_current_state ) return;

    if (pause_target_state) {
        interrupt_maincpu_trigger_trap(pause_trap, 0);
    } else {
        // nothing to do here. Pause will release in pause-loop
    }
}

// ----------------------------------------------------------------------------------
//
int
emu_pause(int state)
{
    if( pause_target_state != pause_current_state ) {
        printd("Attempt to set pause pending state to %d, but state change in progress [current: %d target: %d]\n", state, pause_current_state, pause_target_state );
        return 0;
    }

    if( pause_current_state == state ) return 1;
    // Pause happens at the end of the frame (and within pause, save/load if necessary).
    // Queue the pasue request until then.
    pause_target_state = state;

    return 1;
}

// ----------------------------------------------------------------------------------
//
int
emu_can_change_pause_state()
{
    return (pause_target_state == pause_current_state);
}

// ----------------------------------------------------------------------------------
//
int
emu_is_paused()
{
    return (pause_current_state == 1);
}

// ----------------------------------------------------------------------------------
//
void
disable_pause_and_wait_for_release()
{
    if( !emu_is_paused() ) return;

    pause_target_state = 0;

    // Cycle through a frame sequence to allow pause release to action
    while( emu_is_paused() ) {
        semaphore_wait_for( SEM_FRAME_DONE );
        semaphore_notify( SEM_START_NEXT_FRAME );
    }
}

// ----------------------------------------------------------------------------------
//
static void
wait_for_stable_pause_state()
{
    while( pause_target_state != pause_current_state ) {
        semaphore_wait_for( SEM_FRAME_DONE );
        semaphore_notify( SEM_START_NEXT_FRAME );
    }
}

// ----------------------------------------------------------------------------------
//
static void *
clone_filename( unsigned char *filename ) 
{
    int l = strlen(filename) + 1;
    void *fname = malloc( l );
    memcpy(fname, filename, l );

    return fname;
}

// ----------------------------------------------------------------------------------
//
static void
load_snapshot_trap( WORD addr, void *data )
{
    vsync_suspend_speed_eval();

    if( data ) {
        char *filename = (char *)data;

        if (machine_read_snapshot(filename, 0) < 0) {
            printf("Error: Cannot load snapshot file\n`%s'", filename);
        }

        free(data);
    }
}

// ----------------------------------------------------------------------------------
//
void
emu_load_snapshot( unsigned char *filename )
{
    // We can only load at the end of a frame (ie when paused).
    if( pause_current_state == 1 && (pause_current_state == pause_target_state)) {

        if( !snapshot_filename ) {
          
            semaphore_wait_for( SEM_FRAME_DONE );

            loading = 1;
            snapshot_filename = clone_filename(filename);
            printd("> Loading snapshot from paused state. Tell pause loop to start next frame cycle..\n");
            
            semaphore_notify( SEM_START_NEXT_FRAME );

            printd("> Notify done\n");
            semaphore_wait_for( SEM_SAVE_LOAD_DONE );
        }
    }
}

// ----------------------------------------------------------------------------------
//
static void
save_snapshot_trap( WORD unused_addr, void *data )
{
    if (data) {
        char *filename = (char *)data;
        printd("< ... doing save\n");

        if (machine_write_snapshot(filename, 0, 0, 0) < 0) {
            printf("Screen/snapshot save failed\n");
        }
        free( data ); // Free filename
    }

    vsync_suspend_speed_eval();
}

// ----------------------------------------------------------------------------------
//
void
emu_save_snapshot( unsigned char *filename )
{
    // We can only save at the end of a frame (ie when paused).
    if( pause_current_state == 1 && (pause_current_state == pause_target_state)) {
        if( !snapshot_filename ) {

            semaphore_wait_for( SEM_FRAME_DONE );

            loading = 0;
            snapshot_filename = clone_filename(filename);
            printd("> Saving snapshot from paused state. Tell pause loop to start next frame cycle..\n");

            semaphore_notify( SEM_START_NEXT_FRAME );
            
            printd("> Notify done\n");
            semaphore_wait_for( SEM_SAVE_LOAD_DONE );
        }
    }
}

// ----------------------------------------------------------------------------------
// We can only schedule a quit of the emulator core while PAUSE is in effect
// Return 0 = success, request scheduled
//       -1 = failure, emulator was not paused (but it is forced, so this should never happen)
int
schedule_emulator_quit()
{
    int ret =-1;

    wait_for_stable_pause_state();

    // If pause is not enabled, enable pause and wait until stable before continuing
    if( pause_target_state == 0 || pause_current_state == 0 ) {

        // Pause must be in effect, as that handles the emulator quit
        emu_pause(1);

        wait_for_stable_pause_state();
    }

    // Now that PAUSE is in action, instruct the pause handler that the emulator is to quit
    //
    if( pause_current_state == 1 && (pause_current_state == pause_target_state)) {
        if( !quit_requested ) {

            semaphore_wait_for( SEM_FRAME_DONE );

            quit_requested = 1;

            semaphore_notify( SEM_START_NEXT_FRAME );
        }
        ret = 0;
    } 
    return ret;
}

// ----------------------------------------------------------------------------------
