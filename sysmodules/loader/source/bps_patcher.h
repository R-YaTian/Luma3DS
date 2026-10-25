#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <3ds/types.h>

bool patcherApplyCodeBpsPatch(u8 *code, u32 size, u64 progId);

#ifdef __cplusplus
}
#endif
