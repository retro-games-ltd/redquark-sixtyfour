#ifndef MACHINE_MODEL_H
#define MACHINE_MODEL_H

typedef enum {
    // Models 0..255
    Model_C64   = 0,    // C64 is zero, so that the default is C64 if no model set
    Model_VIC20 = 1,

    Model_Core_Mask       = (1 << 8) - 1,
    Model_Video_Type_Mask = ~Model_Core_Mask,

    Model_Video_Type_NTSC  = 1 << 8,
    Model_Video_Type_PAL   = 1 << 9,

    Model_Strip_Modifier_Mask  = (1 << 20) - 1,
    Model_Modifier_Full_Height = 1 << 20,

    Model_Video_Type_NTSCF = Model_Video_Type_NTSC | Model_Modifier_Full_Height,
    Model_Video_Type_PALF  = Model_Video_Type_PAL  | Model_Modifier_Full_Height,

    Model_C64_NTSC    = Model_C64   | Model_Video_Type_NTSC,
    Model_C64_PAL     = Model_C64   | Model_Video_Type_PAL,
    Model_C64_NTSCF   = Model_C64   | Model_Video_Type_NTSCF,
    Model_C64_PALF    = Model_C64   | Model_Video_Type_PALF,

    Model_VIC20_NTSC  = Model_VIC20 | Model_Video_Type_NTSC,
    Model_VIC20_PAL   = Model_VIC20 | Model_Video_Type_PAL,
    Model_VIC20_NTSCF = Model_VIC20 | Model_Video_Type_NTSCF,
    Model_VIC20_PALF  = Model_VIC20 | Model_Video_Type_PALF,
} machine_model_t;

#endif
