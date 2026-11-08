/*
 * labctl.c - benign IOCTL client for LabDrv.sys.
 *
 * Usage:
 *   labctl.exe ping
 *   labctl.exe version
 *   labctl.exe            (both)
 */

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <string.h>
#include "LabDrv.h"

static BOOL do_ping(HANDLE h)
{
    ULONG magic = 0;
    DWORD bytes = 0;

    if (!DeviceIoControl(h, LAB_IOCTL_PING,
                         NULL, 0,
                         &magic, sizeof(magic),
                         &bytes, NULL)) {
        printf("PING failed: %lu\n", GetLastError());
        return FALSE;
    }

    printf("PING: 0x%08lX (%s)\n", magic,
           (magic == LAB_MAGIC) ? "LAB1" : "unexpected");
    return (magic == LAB_MAGIC);
}

static BOOL do_version(HANDLE h)
{
    LAB_VERSION v;
    DWORD bytes = 0;

    memset(&v, 0, sizeof(v));
    if (!DeviceIoControl(h, LAB_IOCTL_VERSION,
                         NULL, 0,
                         &v, sizeof(v),
                         &bytes, NULL)) {
        printf("VERSION failed: %lu\n", GetLastError());
        return FALSE;
    }

    printf("VERSION: %lu.%lu magic=0x%08lX\n",
           v.Major, v.Minor, v.Magic);
    return (v.Magic == LAB_MAGIC);
}

int main(int argc, char **argv)
{
    HANDLE h;
    const char *cmd = (argc > 1) ? argv[1] : "all";
    BOOL ok = TRUE;

    h = CreateFileW(LAB_USER_PATH,
                    GENERIC_READ | GENERIC_WRITE,
                    0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("open %ls failed: %lu\n", LAB_USER_PATH, GetLastError());
        return 1;
    }

    printf("open %ls OK\n", LAB_USER_PATH);

    if (strcmp(cmd, "ping") == 0) {
        ok = do_ping(h);
    } else if (strcmp(cmd, "version") == 0) {
        ok = do_version(h);
    } else if (strcmp(cmd, "all") == 0) {
        ok = do_ping(h) && do_version(h);
    } else {
        printf("usage: labctl [ping|version|all]\n");
        ok = FALSE;
    }

    CloseHandle(h);
    return ok ? 0 : 1;
}
