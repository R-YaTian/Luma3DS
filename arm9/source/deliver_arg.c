/*
*   This file is part of Luma3DS
*   Copyright (C) 2022 Aurora Wright, TuxSH
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

#include "deliver_arg.h"
#include "utils.h"
#include "memory.h"
#include "config.h"

u8 *loadDeliverArg(void)
{
    static u8 deliverArg[0x1000] = {0};
    static bool deliverArgLoaded = false;

    if (!deliverArgLoaded)
    {
        u32 bootenv = CFG_BOOTENV;  // this register is preserved across reboots
        if ((bootenv & 1) == 0) // true coldboot
        {
            memset(deliverArg, 0, 0x1000);
        }
        else
        {
            u32 mode = bootenv >> 1;
            if (mode == 0) // CTR mode
            {
                memcpy(deliverArg, (const void *)0x20000000, 0x1000);

                u32 testPattern = *(u32 *)(deliverArg + 0x438);
                u32 crc = *(u32 *)(deliverArg + 0x43C);
                u32 expectedCrc = crc32(deliverArg + 0x400, 0x140, 0xFFFFFFFF);
                if (testPattern != 0xFFFF || crc != expectedCrc)
                    memset(deliverArg, 0, 0x1000);
            }
            else // Legacy modes
            {
                // Copy TWL stuff as-is
                copyFromLegacyModeFcram(deliverArg, (const void *)0x20000000, 0x400);
                memset(deliverArg + 0x400, 0, 0xC00);
            }
        }
        deliverArgLoaded = true;
    }

    return deliverArg;
}

void commitDeliverArg(void)
{
    u8 *deliverArg = loadDeliverArg();
    u32 bootenv = CFG_BOOTENV;

    if ((bootenv & 1) == 0) // if true coldboot, set bootenv to "CTR mode reboot"
    {
        bootenv = 1;
        CFG_BOOTENV = 1;
    }

    u32 mode = bootenv >> 1;
    if (mode == 0) // CTR mode
    {
        *(u32 *)(deliverArg + 0x438) = 0xFFFF;
        *(u32 *)(deliverArg + 0x43C) = crc32(deliverArg + 0x400, 0x140, 0xFFFFFFFF);
        memcpy((void *)0x20000000, deliverArg, 0x1000);
    }
    else // Legacy modes
    {
        copyToLegacyModeFcram((void *)0x20000000, deliverArg, 0x400);
    }
}

static bool configureHomebrewAutobootCtr(u8 *deliverArg)
{
    u64 hbldrTid = configData.hbldr3dsxTitleId;
    hbldrTid = hbldrTid == 0 ? HBLDR_DEFAULT_3DSX_TID : hbldrTid; // replicate Loader's behavior
    if ((hbldrTid >> 46) != 0x10) // Not a CTR titleId. Bail out
        return false;

    // Determine whether to load from the SD card or from NAND. We don't support gamecards for this
    u32 category = (hbldrTid >> 32) & 0xFFFF;
    bool isSdApp = (category & 0x10) == 0 && category != 1; // not a system app nor a DLP child
    *(u64 *)(deliverArg + 0x440) = hbldrTid;
    *(u64 *)(deliverArg + 0x448) = isSdApp ? 1 : 0;

    // Tell NS to run the title, and that it's not a title jump from legacy mode
    *(u32 *)(deliverArg + 0x460) = (0 << 1) | (1 << 0);
}

static bool configureHomebrewAutobootTwl(u8 *deliverArg)
{
    memset(deliverArg + 0x000, 0, 0x300); // zero TWL deliver arg params

    // Now onto TLNC (launcher params):
    u8 *tlnc = deliverArg + 0x300;
    memset(tlnc, 0, 0x100);
    memcpy(tlnc, "TLNC", 4);
    tlnc[4] = 1; // version
    tlnc[5] = 0x18; // length of data to calculate CRC over

    *(u64 *)(tlnc + 8) = 0; // old title ID
    *(u64 *)(tlnc + 0x10) = 0x0003000448424C41ull; // new title ID
    // bit4: "skip logo" ; bits2:1: NAND boot ; bit0: valid
    *(u16 *)(tlnc + 0x18) = (1 << 4) | (3 << 1) | (1 << 0);

    *(u16 *)(tlnc + 6) = crc16(tlnc + 8, 0x18, 0xFFFF);

    CFG_BOOTENV = 3;

    return true;
}

bool configureHomebrewAutoboot(void)
{
    u8 *deliverArg = loadDeliverArg();

    u32 bootenv = CFG_BOOTENV;
    u32 mode = bootenv >> 1;

    u32 testPattern = *(u32 *)(deliverArg + 0x438);
    if (mode != 0 || testPattern == 0xFFFF)
        return false; // bail out if this isn't a coldboot/plain reboot

    (void)configureHomebrewAutobootCtr;
    bool ret = configureHomebrewAutobootTwl(deliverArg);

    commitDeliverArg();
    return ret;
}
