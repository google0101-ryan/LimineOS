#pragma once

#define BCS_RINGBUF_BUFFER_TAIL 0x22030
#define BCS_RINGBUF_BUFFER_HEAD 0x22034
#define BCS_RINGBUF_BUFFER_START 0x22038
#define BCS_RINGBUF_BUFFER_CTRL 0x2203C

#define RCS_RINGBUF_BUFFER_TAIL 0x2030
#define RCS_RINGBUF_BUFFER_HEAD 0x2034
#define RCS_RINGBUF_BUFFER_START 0x2038
#define RCS_RINGBUF_BUFFER_CTRL 0x203C

#define NOPID 0x2094

// North registers
#define BLC_PWM_CTL 0x48250
#define BLC_PWM_DATA 0x48254

// South registers
#define IMR 0xC4004
#define SHOTPLUG_CTL 0xC4030
#define GMBUS0 0xC5100
#define GMBUS1 0xC5104
#define GMBUS2 0xC5108
#define GMBUS3 0xC510C
#define PP_CONTROL 0xC7204
#define SBLC_PWM_CTL2 0xC8254
#define HDMI_C_CTL 0xE1150
#define HDMI_D_CTL 0xE1160
#define LVDS_CTL 0xE1180

#define IGFX_MAX_DISPLAYS 3

// Stolen from https://github.com/himanshugoel2797/Cardinal/blob/master/drivers/display/ihd/common/include/display.h
typedef enum 
{
    DisplayType_Unknown,
    DisplayType_LVDS,
    DisplayType_Analog,
    DisplayType_HDMI,
    DisplayType_DisplayPort
} DisplayType;

#define MI_INSTR(opcode, flags) \
    (((opcode) << 23) | (flags))
#define GFX_INSTR(subType, opcode, subOpcode, flags) \
    ((0x3 << 29) | (subType << 27) | ((opcode) << 24) | ((subOpcode) << 16) | (flags))

#define COLOR_BLT                       ((2 << 29) | (0x40 << 22) | 3)