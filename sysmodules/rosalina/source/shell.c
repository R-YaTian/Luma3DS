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

#include <3ds.h>
#include "shell.h"
#include "utils.h"
#include "screen_filters.h"
#include "menus/sysconfig.h"

void handleShellOpened(void)
{
    // Somtimes this is called before Rosalina thread main executes,
    // sometimes not... how fun :))

    // We need to check here if GSP has done its init stuff, in particular
    // clock and reset, otherwise we'll cause core1 to be in a waitstate
    // forever (if we access a GPU reg while the GPU block's clock is off).
    // (GSP does its init before registering its services)
    if (isServiceUsable("gsp::Gpu"))
        ScreenFiltersMenu_RestoreSettings();

    // Volume override, audio routing, LED master brightness override
    SysConfigMenu_RestoreSettings();
}
