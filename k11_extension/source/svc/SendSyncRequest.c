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
#include <string.h>

#include "svc/SendSyncRequest.h"
#include "ipc.h"

#if BUILD_FOR_LEVEL256
static u32 IPC_MakeHeader(u16 command_id, unsigned normal_params, unsigned translate_params)
{
    return ((u32) command_id << 16) | (((u32) normal_params & 0x3F) << 6) | (((u32) translate_params & 0x3F) << 0);
}

typedef enum
{
    ON_SOCKET_ATTACHED = 0,
    ON_SOCKET_DETACHED,
} PlgUdsEvent;

typedef struct
{
    Handle handle;
    u32 *cmdbuf;
    u32 backup[10];
} InterruptiveRequest;

typedef struct
{
    u32 processId;
    bool isLogMode;
    bool isPaused;
    bool signalOnAttachProcess;
    bool signalOnDetachProcess;
} PlgUdsContext;

#define REG32(addr) (*(vu32 *)(PA_PTR(addr)))
#define PU_CHECK_HEADER(idRight, nbParamsRight, nbTranslateRight)                    \
    if (cmdbuf[0] != IPC_MakeHeader((idRight), (nbParamsRight), (nbTranslateRight))) \
    {                                                                                \
        break;                                                                       \
    }

int InterruptiveRequest_Init(InterruptiveRequest *ctx, const char *port, u32 *cmdbuf)
{
    if (ctx == NULL)
        return -1;

    ctx->handle = 0;
    ctx->cmdbuf = cmdbuf;

    SessionInfo *sessionInfo = SessionInfo_FindFirst(port);

    if (sessionInfo == NULL || createHandleForThisProcess(&ctx->handle, &sessionInfo->session->clientSession.syncObject.autoObject) < 0)
        return -1;

    memcpy(ctx->backup, cmdbuf, sizeof(ctx->backup));

    return 0;
}

void InterruptiveRequest_Set(InterruptiveRequest *ctx, u32 id, u32 value)
{
    ctx->cmdbuf[id] = value;
}

Result InterruptiveRequest_Send(InterruptiveRequest *ctx)
{
    return SendSyncRequest(ctx->handle);
}

void InterruptiveRequest_Destroy(InterruptiveRequest *ctx)
{
    memcpy(ctx->cmdbuf, ctx->backup, sizeof(ctx->backup));
    CloseHandle(ctx->handle);
}

Result PLGUDS_SignalEvent(u32 *cmdbuf, PlgUdsEvent event)
{
    Result ret;
    InterruptiveRequest request;
    if (InterruptiveRequest_Init(&request, "plg:UDS", cmdbuf) != 0)
        return -1;

    InterruptiveRequest_Set(&request, 0, IPC_MakeHeader(102, 1, 0));
    InterruptiveRequest_Set(&request, 1, event);

    if ((ret = InterruptiveRequest_Send(&request)) < 0)
        return ret;

    InterruptiveRequest_Destroy(&request);

    return 0;
}

bool PLGUDS_IsConnectedProcess(PlgUdsContext *ctx, u32 pid)
{
    return ctx->processId == pid && !ctx->isLogMode;
}

bool PLGUDS_ProcessCommands(PlgUdsContext *ctx, u32 *cmdbuf, u32 *staticBuf)
{
    bool skip = false;

    switch (cmdbuf[0] >> 16)
    {
    case 3: // Finalize
        break;
    case 14: // GetNodeInfoList
        PU_CHECK_HEADER(14, 0, 6);
        cmdbuf[0] = IPC_MakeHeader(31, 7, 0);
        cmdbuf[7] = staticBuf[1];
        break;
    case 15: // Scan
        PU_CHECK_HEADER(15, 16, 4);
        cmdbuf[0] = IPC_MakeHeader(15, 20, 0);
        break;
    case 16: // SetBeaconData
        PU_CHECK_HEADER(16, 1, 2);
        cmdbuf[0] = IPC_MakeHeader(16, 3, 0);
        break;
    case 17: // GetBeaconData
        PU_CHECK_HEADER(17, 1, 0);
        cmdbuf[0] = IPC_MakeHeader(17, 2, 0);
        cmdbuf[2] = staticBuf[1];
        break;
    case 20: // Receive
        PU_CHECK_HEADER(20, 3, 0);
        cmdbuf[0] = IPC_MakeHeader(20, 4, 0);
        cmdbuf[4] = staticBuf[1];
        break;
    case 23: // Send
        PU_CHECK_HEADER(23, 6, 2);
        cmdbuf[0] = IPC_MakeHeader(23, 8, 0);
        break;
    case 27: // Initialize
        PU_CHECK_HEADER(27, 12, 2);
        cmdbuf[0] = IPC_MakeHeader(27, 14, 0);
        break;
    case 29: // CreateNetwork
        PU_CHECK_HEADER(29, 1, 4);
        cmdbuf[0] = IPC_MakeHeader(29, 5, 0);
        break;
    case 30: // ConnectToNetwork
        PU_CHECK_HEADER(30, 2, 4);
        cmdbuf[0] = IPC_MakeHeader(30, 6, 0);
        break;
    case 31: // GetNodeInfoList2
        PU_CHECK_HEADER(31, 0, 6);
        cmdbuf[0] = IPC_MakeHeader(31, 7, 0);
        cmdbuf[7] = staticBuf[1];
        break;
    case 100: // PLGUDS_Initialize
        PU_CHECK_HEADER(100, 2, 0);
        ctx->processId = cmdbuf[1];
        ctx->isLogMode = (cmdbuf[2] == 1);
        ctx->signalOnAttachProcess = false;
        break;
    case 101: // PLGUDS_Finalize
        PU_CHECK_HEADER(101, 0, 0);
        ctx->processId = 0;
        ctx->isLogMode = false;
        ctx->signalOnAttachProcess = false;
        break;
    case 104: // PLGUDS_SetSignalState
        PU_CHECK_HEADER(104, 1, 0);
        ctx->signalOnAttachProcess = (cmdbuf[1] != 0);
        skip = true; // PLGUDSに送信する必要がない。むしろPLGUDS自体が送信する場合があるので送信すべきでない。
        break;
    case 105: // PLGUDS_SetPause
        PU_CHECK_HEADER(105, 1, 0);
        ctx->isPaused = (cmdbuf[1] != 0);
        skip = true;
        break;
    default:
        break;
    }

    return skip;
}

void PLGUDS_Log(u32 *cmdbuf)
{
    Handle plgUdsHandle;
    SessionInfo *plgUdsInfo = SessionInfo_FindFirst("plg:UDS");
    if (plgUdsInfo != NULL && createHandleForThisProcess(&plgUdsHandle, &plgUdsInfo->session->clientSession.syncObject.autoObject) >= 0)
    {
        u32 header = cmdbuf[0];
        u32 cmdbufOrig[30];
        u32 cmdId = header >> 16;

        if (1 < cmdId && cmdId < 0x24)
        {
            memcpy(cmdbufOrig, cmdbuf, sizeof(cmdbufOrig));

            cmdbuf[0] = IPC_MakeHeader(cmdId, 0, 0);
            SendSyncRequest(plgUdsHandle);

            memcpy(cmdbuf, cmdbufOrig, sizeof(cmdbufOrig));
        }

        CloseHandle(plgUdsHandle);
    }
}
#endif

static inline bool isNdmuWorkaround(const SessionInfo *info, u32 pid)
{
    return info != NULL && strcmp(info->name, "ndm:u") == 0 && hasStartedRosalinaNetworkFuncsOnce && pid >= nbSection0Modules;
}

Result SendSyncRequestHook(Handle handle)
{
    KProcess *currentProcess = currentCoreContext->objectContext.currentProcess;
    KProcessHandleTable *handleTable = handleTableOfProcess(currentProcess);
    u32 pid = idOfProcess(currentProcess);
    KClientSession *clientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, handle);

    u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);
#if BUILD_FOR_LEVEL256
    u32 *staticBuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x180);
    static PlgUdsContext plgUdsContext;
    plgUdsContext.signalOnDetachProcess = false;
#endif
    bool skip = false;
    Result res = 0;

     // not the exact same test but it should work
    bool isValidClientSession = clientSession != NULL && strcmp(classNameOfAutoObject(&clientSession->syncObject.autoObject), "KClientSession") == 0;

    if(isValidClientSession)
    {
#if BUILD_FOR_LEVEL256
        // UDS Hook
        SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);

        if (info != NULL)
        {
            // plg:UDS使用時
            if (PLGUDS_IsConnectedProcess(&plgUdsContext, pid))
            {
                // NDMのLocal-Communication排他処理をブロック
                if (strcmp(info->name, "ndm:u") == 0 && !plgUdsContext.isPaused)
                {
                    if (cmdbuf[0] == 0x10042 && cmdbuf[1] == 2 /* Local-Communication */)
                    {
                        cmdbuf[1] = 1 /* Infrastructure */;
                    }
                }

                // SocketのAttachProcess時にToo many processesエラーを回避
                if (strcmp(info->name, "soc:U") == 0)
                {
                    if (plgUdsContext.signalOnAttachProcess)
                    {
                        // AttachProcess
                        if (cmdbuf[0] == 0x10044)
                        {
                            if (PLGUDS_SignalEvent(cmdbuf, ON_SOCKET_ATTACHED) < 0)
                            {
                                *(u32 *)0xDEADBEEF = 0xDEADAAAA;
                            }
                        }

                        // DetachProcess
                        if (cmdbuf[0] == 0x190000)
                        {
                            plgUdsContext.signalOnDetachProcess = true;
                        }
                    }
                }
            }

            // PLGUDS
            if (strcmp(info->name, "plg:UDS") == 0)
            {
                skip = PLGUDS_ProcessCommands(&plgUdsContext, cmdbuf, staticBuf);
            }
        }

        // Log mode
        if (plgUdsContext.isLogMode && info != NULL && strcmp(info->name, "nwm::UDS") == 0)
        {
            PLGUDS_Log(cmdbuf);
        }
#endif
        switch (cmdbuf[0])
        {
            case 0x10042:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(isNdmuWorkaround(info, pid))
                {
                    cmdbuf[0] = 0x10040;
                    cmdbuf[1] = 0;
                    skip = true;
                }

                break;
            }

            case 0x10082:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0)) // GetConfigInfoBlk2
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x10800:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && strcmp(info->name, "err:f") == 0) // Throw
                    skip = doErrfThrowHook(cmdbuf);

                break;
            }

            case 0x20000:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0)) // SecureInfoGetRegion
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x20002:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(isNdmuWorkaround(info, pid))
                {
                    cmdbuf[0] = 0x20040;
                    cmdbuf[1] = 0;
                    skip = true;
                }

                break;
            }

            case 0x50100:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "srv:") == 0 || (GET_VERSION_MINOR(kernelVersion) < 39 && strcmp(info->name, "srv:pm") == 0)))
                {
                    char name[9] = { 0 };
                    memcpy(name, cmdbuf + 1, 8);
#if BUILD_FOR_LEVEL256
                    if (PLGUDS_IsConnectedProcess(&plgUdsContext, pid) && !plgUdsContext.isPaused && strcmp(name, "nwm::UDS") == 0)
                    {
                        const char *plgUds = "plg:UDS";
                        memcpy(cmdbuf + 1, plgUds, 8);
                        strncpy(name, plgUds, 8);
                        cmdbuf[3] = 7;
                    }
#endif
                    skip = true;
                    res = SendSyncRequest(handle);
                    if(res == 0)
                    {
                        KClientSession *outClientSession;

                        outClientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
                        if(outClientSession != NULL)
                        {
                            if(strcmp(classNameOfAutoObject(&outClientSession->syncObject.autoObject), "KClientSession") == 0)
                                SessionInfo_Add(outClientSession->parentSession, name);
                            outClientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&outClientSession->syncObject.autoObject);
                        }
                    }
                    else
                    {
                        // Prior to 11.0 kernel didn't zero-initialize output handles, and thus
                        // you could accidentaly close things like the KAddressArbiter handle by mistake...
                        cmdbuf[3] = 0;
                    }
                }

                break;
            }

            case 0x80040:
            {
                if(!hasStartedRosalinaNetworkFuncsOnce)
                    break;
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                skip = isNdmuWorkaround(info, pid); // SuspendScheduler
                if(skip)
                    cmdbuf[1] = 0;
                break;
            }

            case 0x90000:
            {
                if(!hasStartedRosalinaNetworkFuncsOnce)
                    break;
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(isNdmuWorkaround(info, pid)) // ResumeScheduler
                {
                    cmdbuf[0] = 0x90040;
                    cmdbuf[1] = 0;
                    skip = true;
                }
                break;
            }

            case 0x00C0080: // srv: publishToSubscriber
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);

                if (info != NULL && strcmp(info->name, "srv:") == 0 && cmdbuf[1] == 0x1002)
                {
                    // Wake up application thread
                    PLG__WakeAppThread();
                    cmdbuf[0] = 0xC0040;
                    cmdbuf[1] = 0;
                    skip = true;
                }
                break;
            }

            case 0x00D0080: // APT:ReceiveParameter
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);

                if (info != NULL && strncmp(info->name, "APT:", 4) == 0 && cmdbuf[1] == 0x300)
                {
                    res = SendSyncRequest(handle);
                    skip = true;

                    if (res >= 0)
                    {
                        u32 plgStatus = PLG_GetStatus();
                        u32 command = cmdbuf[3];

                        if ((plgStatus == PLG_CFG_RUNNING && command == 3) // COMMAND_RESPONSE
                        || (plgStatus == PLG_CFG_INHOME && (command >= 10 || command <= 12)))  // COMMAND_WAKEUP_BY_EXIT || COMMAND_WAKEUP_BY_PAUSE
                            PLG_SignalEvent(PLG_CFG_HOME_EVENT);
                    }
                }
                break;
            }

            case 0x2E0040: // mcu::RTC SetPowerLedPattern
            {
                // LED master brightness override set to 0%: prevent ptm from setting
                // sleep "breathing" pattern so that the led can stay off/in auto mode

                // We deliberately leave the POWER LED setting as "auto" instead of "off"
                // so that the red POWER LED appears regardless of setting.
                // Note that the red POWER LED is not dimmable.
                if (cfwInfo.ledMasterBrightnessOverride != 0 || (u8)cmdbuf[1] != 2) // sleep "breathing" pattern
                    break;

                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if (info != NULL && strcmp(info->name, "mcu::RTC") == 0)
                    cmdbuf[1] = (cmdbuf[1] & ~0xFF) | 0; // "auto" LED pattern
                break;
            }

            case 0x300040: // mcu::RTC SetLedMasterBrightness
            {
                // LED master brightness override set to 0% (enabled, value 0): prevent ptm from
                // changing the LED brightness when setting up sleep mode
                if (cfwInfo.ledMasterBrightnessOverride != 0)
                    break;

                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if (info != NULL && strcmp(info->name, "mcu::RTC") == 0)
                {
                    cmdbuf[0] = 0x300040;
                    cmdbuf[1] = 0;
                    skip = true;
                }
                break;
            }

            case 0x4010082:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0)) // GetConfigInfoBlk4
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x4020082:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0)) // GetConfigInfoBlk8
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x8010082:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0)) // GetConfigInfoBlk4
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x8020082:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && strcmp(info->name, "cfg:i") == 0) // GetConfigInfoBlk8
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x4060000:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession); // SecureInfoGetRegion
                if(info != NULL && (strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }

            case 0x8160000:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession); // SecureInfoGetRegion
                if(info != NULL && strcmp(info->name, "cfg:i") == 0)
                    skip = doLangEmu(&res, cmdbuf);

                break;
            }
        }
    }

    if(clientSession != NULL)
        clientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&clientSession->syncObject.autoObject);

    res = skip ? res : SendSyncRequest(handle);

#if BUILD_FOR_LEVEL256
    if (plgUdsContext.signalOnDetachProcess)
    {
        if (PLGUDS_SignalEvent(cmdbuf, ON_SOCKET_DETACHED) < 0)
        {
            *(u32 *)0xDEADBEEF = 0xDEADBBBB;
        }
    }
#endif

    return res;
}
