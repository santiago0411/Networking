#pragma once

#if defined(NET_WINDOWS)
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#endif

#include "NetCommon.h"
#include "NetMessage.h"
#include "NetServer.h"