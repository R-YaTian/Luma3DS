/*
*   This file is part of Luma3DS
*   Copyright (C) 2023 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <3ds/types.h>

#define CONFIG(a)        (((cfg->config >> (a)) & 1) != 0)
#define MULTICONFIG(a)   ((cfg->multiConfig >> (2 * (a))) & 3)
#define BOOTCONFIG(a, b) ((cfg->bootConfig >> (a)) & (b))

enum singleOptions
{
    AUTOBOOTEMU = 0,
    LOADEXTFIRMSANDMODULES,
    PATCHGAMES,
    REDIRECTAPPTHREADS,
    PATCHVERSTRING,
    SHOWGBABOOT,
    PATCHUNITINFO,
    ENABLEDSIEXTFILTER,
    DISABLEARM11EXCHANDLERS,
    ENABLESAFEFIRMROSALINA,
};

enum multiOptions
{
    DEFAULTEMU = 0,
    BRIGHTNESS,
    SPLASH,
    PIN,
    NEWCPU,
    AUTOBOOTMODE,
};

// MCU-stored config flags (LumaMcuConfig::flags)
#define MCUCFG_FLAG_ENHANCE_TOP_SCREEN_COLORS       BIT(0)
#define MCUCFG_FLAG_ENHANCE_BOTTOM_SCREEN_COLORS    BIT(1)
#define MCUCFG_FLAGS_AUDIO_ROUTING_SHIFT            2
#define MCUCFG_FLAGS_AUDIO_ROUTING_MASK             (3u << MCUCFG_FLAGS_AUDIO_ROUTING_SHIFT)
#define MCUCFG_FLAG_TEMPERATURE_IN_FAHRENHEIT       BIT(4)

typedef enum AudioRouting
{
    AUDIO_ROUTING_AUTO = 0,
    AUDIO_ROUTING_SPEAKERS,
    AUDIO_ROUTING_HEADPHONES,
} AudioRouting;

// Percentage overrides: bit7 is "disabled" flag, bits 0-6 are the percentage (0 to 100, other values meaning "not set").
// Being enabled does not imply having a value.
#define PCT_OVERRIDE_UNSET      0xFF // disabled and not set

static inline bool PctOverride_IsEnabled(u8 v)      { return (v & 0x80) == 0; }
static inline u8 PctOverride_GetPercentage(u8 v)    { return v & 0x7F; }
static inline bool PctOverride_HasValue(u8 v)       { return PctOverride_GetPercentage(v) <= 100; }
static inline bool PctOverride_IsActive(u8 v)       { return PctOverride_IsEnabled(v) && PctOverride_HasValue(v); }
static inline u8 PctOverride_SetEnabled(u8 v, bool enabled) { return enabled ? (v & ~0x80) : (v | 0x80); }
static inline u8 PctOverride_SetPercentage(u8 v, u8 pct)    { return (v & 0x80) | (pct > 100 ? 100 : pct); }

// Stored at the end of the MCU free register area (not checksummed), see arm9 CfgDataMcu. Changes are
// written back immediately to the MCU (thus persisted across reboots) by LumaConfig_SaveMcuConfig.
typedef struct LumaMcuConfig
{
    u8 flags;
    u8 volumeSliderOverride;
    s8 volumeSliderOverrideStereoBalance;   // -100 (left) to +100 (right), only applies to I2S1
    u8 ledMasterBrightnessOverride;
} LumaMcuConfig;

extern LumaMcuConfig lumaMcuConfig;

static inline AudioRouting LumaConfig_GetAudioRouting(void)
{
    return (AudioRouting)((lumaMcuConfig.flags & MCUCFG_FLAGS_AUDIO_ROUTING_MASK) >> MCUCFG_FLAGS_AUDIO_ROUTING_SHIFT);
}

void LumaConfig_LoadMcuConfig(void);
Result LumaConfig_SaveMcuConfig(void);
void LumaConfig_SetMcuConfigFlags(u8 mask, u8 value);

void LumaConfig_ConvertComboToString(char *out, u32 combo);
Result LumaConfig_SaveSettings(void);
void LumaConfig_RequestSaveSettings(void);
