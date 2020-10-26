#include <stdio.h>
#include <string.h>
#include "sem.h"
#include "machine.h"
#include "resources.h"
#include "vdrive/vdrive-internal.h"
#include "diskimage.h"
#include "cartridge.h"
#include "attach.h"
#include "tape.h"
#include "kbdbuf.h"
#include "emucore.h"
#include "sid/sid.h"
#include "emucore.h"
#include "uitraps.h"
#include "sound.h"
#include "resources.h"
#include "screenshot.h"
#include "datasette.h"
#include "c64/c64-snapshot.h"
#include "vicii.h"

// Import emu_load_params_t definition
#include "emu_bind_decl.h"

int emu_load( const char *filename, const char *ext ) {

    int elen = strlen(ext);

#ifdef HANDLE_1581
    // Check d81 files and select the correct drive emulation
    if( elen == 4 && 
        ext[elen-4] == '.' &&
        (0x20 | ext[elen-3]) == 'd' &&
                ext[elen-2]  == '8' &&
                ext[elen-1]  == '1' ) {

        resources_set_string("Drive8Type", "1581" );
        //printf("Setting 1581\n");
    } else {
        //printf("Setting 1541\n");
        resources_set_string("Drive8Type", "1541" );
    }
#endif

    return autostart_autodetect( filename, NULL, 0, 0 );
}

int emu_load_in_progress() {
    return autostart_in_progress(); // XXX Relies on a patch to Vice to return FALSE if AUTOSTART_ERROR is true
}


void emu_load_cancel( emu_load_params_t *p )
{
    resources_set_int("DriveTrueEmulation", p->accurate_loading ? 1 : 0 );
    resources_set_int("WarpMode", 0); // Make sure warp mode is cancelled - can be left on
}

emu_load_status_t * emu_load_start( emu_load_params_t *params )
{
    int do_reset = 1;

    static emu_load_status_t load_status = {0};
    memset( &load_status, 0, sizeof(emu_load_status_t) );

    if( params->filename[0] == '\0' ) return NULL;

    int flen = strlen( params->file_extension );

    resources_set_int("RAMInitStartValue", 0 );

    if( params->media_type == Emu_Media_Cart || // FIXME Emu_Media_Cart is a hangover from experimental type tagging (feature/multi-file-cart)
        ( flen >= 3 && 
             params->file_extension[flen-4] == '.' &&
            (params->file_extension[flen-3] | 0x20) == 'c' &&
            (params->file_extension[flen-2] | 0x20) == 'r' 
            //&& (params->file_extension[flen-1] | 0x20) == 't'  // Allow crt cr2 cr4 cr6 cra crb (ie crx)
            ) ) {

        if( params->title_id > 0 ) {
            // Set inital RAM memory state so (patched) cartridge knows which
            // title to auto-pick from the menu.
            resources_set_int("RAMInitStartValue", params->title_id );
        }

        core_cartridge_attach_image( params->filename ); // Per-core specific cartridge insert - Handles different cart types

        do_reset = 1;
        load_status.display_off_for_frames = 4;

    } else { // Autoload

        if( params->is_external ) {
            // For loading stability, force TrueDrive on and use Emulator Warp mode to
            // speed up loading in addition to the AutostartWarp setting.
            resources_set_int("AutostartHandleTrueDriveEmulation", 0);
            resources_set_int("AttachDevice8Readonly", params->readonly     ? 1 : 0 ); 
            resources_set_int("AutostartWarp",         params->load_at_warp ? 1 : 0 );
        }
        resources_set_int("DriveTrueEmulation", params->accurate_loading ? 1 : 0 );

        // Incase autoload fails...
        load_status.display_off_for_frames = 8;

        // filename len must be at least 4 (.ext)
        if( flen >= 4 && emu_load( params->filename, params->file_extension ) < 0 ) {
            // Error
            fprintf(stderr, "could not load image file [%s]\n", params->filename );
            resources_set_int("WarpMode", 0); // Make sure warp is cancelled - can be left on
        } else {
            do_reset = 0;
            load_status.is_loading = 1;
        }
    }

    // We're not autoloading, so reset machine to start. This invokes "BASIC", or a cartridge program.
    if( do_reset ) emu_core_reset( Emu_Reset_Hard );

    return &load_status;
}

int emu_insert_media( emu_load_params_t *params )
{
    static emu_load_status_t load_status = {0};

    if( params->filename[0] == '\0' ) return -1;

    memset( &load_status, 0, sizeof(emu_load_status_t) );

    if( params->media_type == Emu_Media_Cart ) core_cartridge_attach_image( params->filename );
    if( params->media_type == Emu_Media_Tape ) tape_image_attach(1, params->filename );
    if( params->media_type == Emu_Media_Disk ) {
        resources_set_int("DriveTrueEmulation", params->accurate_loading ? 1 : 0 );
        resources_set_int("AttachDevice8Readonly", params->readonly     ? 1 : 0 ); 
        file_system_attach_disk( 8, params->filename );
    }

    return 0;
}

int emu_eject_media( emu_media_type_t mtype )
{
    if( mtype == Emu_Media_Disk ) file_system_detach_disk(-1); // All disks
    if( mtype == Emu_Media_Cart ) cartridge_detach_image(-1);
    if( mtype == Emu_Media_Tape ) tape_image_detach(1);
}

int emu_create_blank_disk_image( char *fullpath, char *diskname ) {
    return vdrive_internal_create_format_disk_image( fullpath, diskname, DISK_IMAGE_TYPE_D64 );
}

void emu_core_reset( emu_reset_type_t rt ) {

    switch( rt )
    {
        case Emu_Reset_Hard:
            resources_set_int("Sound", 1 );
            machine_trigger_reset( MACHINE_RESET_MODE_HARD );
            break;

        case Emu_Reset_Soft:
            resources_set_int("Sound", 1 );
            machine_trigger_reset( MACHINE_RESET_MODE_SOFT );
            break;

        case Emu_Reset_Freeze:
            core_cartridge_trigger_freeze();
            break;
    }
}

void emu_wait_for_frame() {
    semaphore_wait_for( SEM_FRAME_DONE );
}

void emu_start_frame() {
    semaphore_notify( SEM_START_NEXT_FRAME );
}

int emu_screenshot_save( char *filename ) {
    return screenshot_save("PNG", filename, vicii_get_canvas() );
}

void emu_cleanup() {
    resources_set_int("CartridgeReset", 0 );

    tape_image_detach(1);
    file_system_detach_disk(-1); // All disks
    cartridge_detach_image(-1);  // All carts
    delete_temporary_files();

    resources_set_string("Drive8Type", "1541" ); // FIXME May need to be taken from a "default disk" setting
}

void emu_sound_disable() {
    sound_set_warp_mode(1);
}

void emu_sound_enable() {
    sound_set_warp_mode(0);
}

void emu_mute()
{
    sound_clear(); // Clear current buffer - Must be done first!

    resources_set_int("SoundVolume", 0 );
    sound_set_warp_mode(1);
}

void emu_unmute( )
{
    sound_clear(); // Clear current buffer - Must be done first!

    resources_set_int("SoundVolume", 100 );
    sound_set_warp_mode(0);
}

void emu_volume( unsigned int vol ) {
    if( vol > 100 ) vol = 100;
    resources_set_int("SoundVolume", vol );
}

int emu_set_settingi( const char *param, int v) {
    return resources_set_int( param, v );
}

int emu_set_settings( const char *param, const char *v) {
    return resources_set_string( param, v );
}

static int drive_led = 0;

int emu_drive_led_status() {
    return drive_led;
}

void 
ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int pwm2)
{
    static int LED = 0;

    uint32_t status = 0;

    if( pwm1 > 100 ) status |= 1;
    if( pwm2 > 100 ) status |= 2;

    if(  status && !LED ) { drive_led = 1; }
    if( !status &&  LED ) { drive_led = 0; }

    LED = status;
}

static int tape_counter = 0;
static int tape_running = 0;

void ui_display_tape_counter(int counter)
{
    tape_counter = counter;
}

void ui_display_tape_motor_status(int motor)
{
    tape_running = motor ? 1 : 0;
}

int
emu_cassette_control( emu_media_cassette_command_t command )
{
    int r = 0;
    switch( command ) {
        case Emu_Media_Cassette_Stop:    r = datasette_control( DATASETTE_CONTROL_STOP          ); break;
        case Emu_Media_Cassette_Start:   r = datasette_control( DATASETTE_CONTROL_START         ); break;
        case Emu_Media_Cassette_Forward: r = datasette_control( DATASETTE_CONTROL_FORWARD       ); break;
        case Emu_Media_Cassette_Rewind:  r = datasette_control( DATASETTE_CONTROL_REWIND        ); break;
        case Emu_Media_Cassette_Advance: r = datasette_control( DATASETTE_CONTROL_ADVANCE       ); break;
 
        case Emu_Media_Cassette_Reset:   r = datasette_control( DATASETTE_CONTROL_RESET_COUNTER ); break;
        case Emu_Media_Cassette_Counter: r = tape_counter; break;
        case Emu_Media_Cassette_Motor:   r = tape_running; break;
        case Emu_Media_Cassette_Command:
            {
                switch( datasette_control( DATASETTE_CONTROL_GET_CMD ) ) {
                    case DATASETTE_CONTROL_STOP:    r = Emu_Media_Cassette_Stop;    break;
                    case DATASETTE_CONTROL_START:   r = Emu_Media_Cassette_Start;   break;
                    case DATASETTE_CONTROL_FORWARD: r = Emu_Media_Cassette_Forward; break;
                    case DATASETTE_CONTROL_REWIND:  r = Emu_Media_Cassette_Rewind;  break;
                    default: r = Emu_Media_Cassette_Unknown; break;
                }
            }
            break;
    }
    return r;
}
