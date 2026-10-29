/*
*   This file is part of Luma3DS
*   Copyright (C) 2016-2020 Aurora Wright, TuxSH
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

#include "utils.h"
#include "csvc.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <3ds.h>

void formatMemoryPermission(char *outbuf, MemPerm perm)
{
    if (perm == MEMPERM_DONTCARE)
    {
        strcpy(outbuf, "???");
        return;
    }

    outbuf[0] = perm & MEMPERM_READ ? 'r' : '-';
    outbuf[1] = perm & MEMPERM_WRITE ? 'w' : '-';
    outbuf[2] = perm & MEMPERM_EXECUTE ? 'x' : '-';
    outbuf[3] = '\0';
}

void formatUserMemoryState(char *outbuf, MemState state)
{
    static const char *states[12] =
    {
        "Free",
        "Reserved",
        "IO",
        "Static",
        "Code",
        "Private",
        "Shared",
        "Continuous",
        "Aliased",
        "Alias",
        "AliasCode",
        "Locked"
    };

    strcpy(outbuf, state > 11 ? "Unknown" : states[state]);
}

u32 formatMemoryMapOfProcess(char *outbuf, u32 bufLen, Handle handle)
{
    u32         maxLineSize = 35 + (handle == CUR_PROCESS_HANDLE ? 15 : 0);
    u32         address = 0;
    u32         posInBuffer = 0;
    u32         maxPosInBuffer = bufLen - maxLineSize; // 35 is the maximum length of a formatted region
    MemInfo     memi;
    PageInfo    pagei;
    char        pabuf[32];
    char        permbuf[8];
    char        statebuf[16];

    s64 TTBCR;
    svcGetSystemInfo(&TTBCR, 0x10002, 0);

    while (address < (1u << (32 - (u32)TTBCR)) // Limit to check for regions
        && posInBuffer < maxPosInBuffer
        && R_SUCCEEDED(svcQueryProcessMemory(&memi, &pagei, handle, address)))
    {
        // Update the address for next region
        address = memi.base_addr + memi.size;

        // If region isn't FREE then add it to the list
        if (memi.state != MEMSTATE_FREE)
        {
            if (handle == CUR_PROCESS_HANDLE)
            {
                u32 pa = svcConvertVAToPA((void *)memi.base_addr, false);
                sprintf(pabuf, " (PA %08lx)", pa);
            }
            else
                pabuf[0] = '\0';

            formatMemoryPermission(permbuf, memi.perm);
            formatUserMemoryState(statebuf, memi.state);

            posInBuffer += sprintf(outbuf + posInBuffer, "%08lx - %08lx%s %s %s\n",
                memi.base_addr, address, pabuf, permbuf, statebuf);
        }
    }

    svcCloseHandle(handle);
    return posInBuffer;
}

void calendarTimeFromMsSince1900(CalendarTime *out, u64 msSince1900)
{
    u32 daysSince1900 = (u32)(msSince1900 / MS_IN_A_DAY);
    u32 msRem = (u32)(msSince1900 % MS_IN_A_DAY);

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Adapted from: https://github.com/cassioneri/eaf/blob/main/algorithms/neri_schneider.hpp

    // SPDX-License-Identifier: MIT
    // SPDX-FileCopyrightText: 2022 Cassio Neri <cassio.neri@gmail.com>
    // SPDX-FileCopyrightText: 2022 Lorenz Schneider <schneider@em-lyon.com>

    // Shift and correction constants.
    const u32 s = 82;
    const u32 K = 719468 + 146097 * s;
    const u32 L = 400 * s;

    // Rata die shift.
    const u32 N = (daysSince1900 - 25567) + K; // [modified (inlined function call, 1900 epoch shift applied)]

    // Century.
    const u32 N_1 = 4 * N + 3;
    const u32 C   = N_1 / 146097;
    const u32 N_C = N_1 % 146097 / 4;

    // Year.
    const u32 N_2 = 4 * N_C + 3;
    const u64 P_2 = 2939745ull * N_2;
    const u32 Z   = (u32)(P_2 / 4294967296ull);
    const u32 N_Y = (u32)(P_2 % 4294967296ull) / 2939745 / 4;
    const u32 Y   = 100 * C + Z;

    // Month and day.
    const u32 N_3 = 2141 * N_Y + 197913;
    const u32 M   = N_3 / 65536;
    const u32 D   = N_3 % 65536 / 2141;

    // Map. (Notice the year correction, including type change.)
    const u32 J   = N_Y >= 306;
    const s32 Y_G = (Y - L) + J;
    const u32 M_G = J ? M - 12 : M;
    const u32 D_G = D + 1;
    /////////////////////////////////////////////////////////////////////////////////////////////

    u32 secs = msRem / 1000;
    u32 mins = secs / 60;

    out->year       = Y_G;
    out->month      = (s8)M_G;
    out->day        = (s8)D_G;
    out->weekday    = (s8)((1 + daysSince1900) % 7); // 1900-01-01 was a Monday
    out->hour       = (s8)(mins / 60);
    out->minute     = (s8)(mins % 60);
    out->second     = (s8)(secs % 60);
    out->ms         = (s16)(msRem % 1000);
}

int calendarTimeToString(char *out, const CalendarTime *t)
{
    static const char *const shortWeekdayStrs[7] = { "周日", "周一", "周二", "周三", "周四", "周五", "周六" };

    s32 yyyy = CLAMP(t->year, 0, 9999);
    return sprintf(
        out, "%04ld/%02d/%02d %s %02d:%02d",
        yyyy, t->month, t->day, shortWeekdayStrs[t->weekday], t->hour, t->minute
    );
}

int dateTimeToString(char *out, u64 msSince1900, bool filenameFormat)
{
    CalendarTime t;
    calendarTimeFromMsSince1900(&t, msSince1900);

    if (filenameFormat)
        return sprintf(out, "%04ld-%02d-%02d_%02d-%02d-%02d.%03d", t.year, t.month, t.day, t.hour, t.minute, t.second, t.ms);
    else
        return sprintf(out, "%04ld-%02d-%02d %02d:%02d:%02d", t.year, t.month, t.day, t.hour, t.minute, t.second);
}

int floatToString(char *out, float f, u32 precision, bool pad)
{
    // Floating point stuff is cringe
    if (isnanf(f))
        return sprintf(out, "NaN");
    else if (isinff(f) && f >= -0.0f)
        return sprintf(out, "inf");
    else if (isinff(f))
        return sprintf(out, "-inf");

    static const u64 pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
    precision = precision >= 6 ? 6 : precision; // result inaccurate after 1e-6

    u64 mult = pow10[precision];
    double f2 = fabs((double)f) * mult + 0.5;
    u64 f3 = (u64)f2;
    const char *sign = (f >= 0.0f || f3 == 0) ? "" : "-";

    u64 intPart = f3 / mult;
    u64 fracPart = f3 % mult;

    if (pad)
        return sprintf(out, "%s%llu.%0*llu", sign, intPart, (int)precision, fracPart);
    else
    {
        int n = sprintf(out, "%s%llu", sign, intPart);
        if (fracPart == 0)
            return n;

        n += sprintf(out + n, ".%0*llu", (int)precision, fracPart);

        int n2 = n - 1;
        while (out[n2] == '0')
            out[n2--] = '\0';
        return n2;
    }
}
