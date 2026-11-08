#pragma once

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#include <winioctl.h>
#endif

#define LAB_DEVICE_NAME   L"\\Device\\LabDrv"
#define LAB_SYMLINK_NAME  L"\\DosDevices\\LabDrv"
#define LAB_USER_PATH     L"\\\\.\\LabDrv"

#define LAB_MAGIC         0x4C414231UL   /* "LAB1" */

#define LAB_IOCTL_PING \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LAB_IOCTL_VERSION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _LAB_VERSION {
    ULONG Size;
    ULONG Major;
    ULONG Minor;
    ULONG Magic;
} LAB_VERSION, *PLAB_VERSION;
