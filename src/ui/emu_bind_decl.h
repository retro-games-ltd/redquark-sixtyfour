/*
 * THEC64
 * Copyright (C) 2020 Chris Smith
 *
 * This confidential and proprietary software may be used only
 * as authorised by a licensing agreement from Chris Smith.
 * Unauthorized copying of this file, via any medium, is 
 * strictly prohibited.
 *
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from Chris Smith.
 */
#ifndef TSF_EMULATOR_BIND_DECL_H
#define TSF_EMULATOR_BIND_DECL_H

#include "machine_model.h"

#define EMU_EXPORT_ID(c) const char const *emu_id = (char *)&(c.id_str[0])

typedef struct {
    int width;
} emu_screen_scaling_t;

#define SCALE_WIDTHS_MAX 3
typedef struct {
    int         screen_number;
    machine_model_t model;
    int         _start_line;
    int         pixel_width;
    int         pixel_height;
    int         pixel_depth;

    int         shot_offset_x;
    int         shot_offset_y;
    int         shot_width;
    int         shot_height;

    emu_screen_scaling_t scale  [ SCALE_WIDTHS_MAX ];
    emu_screen_scaling_t vkshift[ SCALE_WIDTHS_MAX ];
} emu_screen_t;

typedef struct {
    char const * const * const filename_flags;
    char const * const * const tsg_flags;
} emu_configuration_t;

#define EMU_CAPABILITIES_ID_MAX 5
typedef struct {
    machine_model_t     core_id;
    char                id_str[EMU_CAPABILITIES_ID_MAX + 1]; // Unique ID
    emu_screen_t        screens[8];
    int                 screen_count;
    int                 active_screen;
    emu_configuration_t configuration;
} emu_capabilities_t;

typedef struct {
    int is_loading;      // True if loading has started
    int display_off_for_frames; // The number of frames to keep screen blank
} emu_load_status_t;

typedef enum {
    Emu_Media_None = 0,
    Emu_Media_Disk = 1,
    Emu_Media_Cart = 2,
    Emu_Media_Tape = 3,
    Emu_Media_Misc = 4,

    Emu_Media_First = Emu_Media_Disk,
    Emu_Media_Max   = Emu_Media_Misc,
    Emu_Media_Count = Emu_Media_Max + 1,
} emu_media_type_t;

typedef enum {
    Emu_Media_Cassette_Stop    = 0,
    Emu_Media_Cassette_Start   = 1,
    Emu_Media_Cassette_Forward = 2,
    Emu_Media_Cassette_Rewind  = 3,
    Emu_Media_Cassette_Reset   = 4,
    Emu_Media_Cassette_Counter = 5,
    Emu_Media_Cassette_Motor   = 6,
    Emu_Media_Cassette_Advance = 7,
    Emu_Media_Cassette_Command = 8,
    Emu_Media_Cassette_None    = 98,
    Emu_Media_Cassette_Unknown = 99,
} emu_media_cassette_command_t;

typedef struct {
    emu_media_type_t media_type;
    char filename[1024];
    char file_extension[10]; // XXX TEMPORARILY INCLUDES THE LEADING DOT XXX
    int  title_id;           // For media that may have numerous titles, indexed by ID (such as malti-game cartridges)
    int  is_external;
    int  load_at_warp;
    int  readonly;
    int  accurate_loading;
} emu_load_params_t;

typedef enum {
    Emu_Reset_Hard   = 0,
    Emu_Reset_Soft   = 1,
    Emu_Reset_Freeze = 2,
} emu_reset_type_t;

#endif
