/*
 * LabDrv.c - benign WDM lab driver.
 *
 * Creates \Device\LabDrv and two read-only IOCTLs:
 *   LAB_IOCTL_PING    -> returns LAB_MAGIC
 *   LAB_IOCTL_VERSION -> returns LAB_VERSION
 *
 * It intentionally contains no memory read/write, process, MSR, port, or
 * hiding functionality.
 */

#ifndef _KERNEL_MODE
#define _KERNEL_MODE 1
#endif

#include <ntddk.h>
#include "LabDrv.h"

#define LAB_DRIVER_VERSION_MAJOR 1
#define LAB_DRIVER_VERSION_MINOR 0

static PDEVICE_OBJECT g_Device;
static BOOLEAN g_SymLinkCreated;

static NTSTATUS LabCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS LabDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION sp;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG code;
    ULONG outlen;
    ULONG info = 0;
    PVOID buffer;

    UNREFERENCED_PARAMETER(DeviceObject);

    sp = IoGetCurrentIrpStackLocation(Irp);
    code = sp->Parameters.DeviceIoControl.IoControlCode;
    outlen = sp->Parameters.DeviceIoControl.OutputBufferLength;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    switch (code) {
    case LAB_IOCTL_PING:
        if (outlen < sizeof(ULONG) || buffer == NULL) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        *(PULONG)buffer = LAB_MAGIC;
        info = sizeof(ULONG);
        status = STATUS_SUCCESS;
        break;

    case LAB_IOCTL_VERSION:
        if (outlen < sizeof(LAB_VERSION) || buffer == NULL) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        {
            PLAB_VERSION v = (PLAB_VERSION)buffer;
            v->Size = sizeof(LAB_VERSION);
            v->Major = LAB_DRIVER_VERSION_MAJOR;
            v->Minor = LAB_DRIVER_VERSION_MINOR;
            v->Magic = LAB_MAGIC;
        }
        info = sizeof(LAB_VERSION);
        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

static VOID LabUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    if (g_SymLinkCreated) {
        UNICODE_STRING symLink;
        RtlInitUnicodeString(&symLink, LAB_SYMLINK_NAME);
        IoDeleteSymbolicLink(&symLink);
        g_SymLinkCreated = FALSE;
    }

    if (g_Device != NULL) {
        IoDeleteDevice(g_Device);
        g_Device = NULL;
    }

    DbgPrint("[LabDrv] unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING symLink;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&deviceName, LAB_DEVICE_NAME);
    status = IoCreateDevice(DriverObject,
                            0,
                            &deviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &g_Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_Device->Flags |= DO_BUFFERED_IO;

    RtlInitUnicodeString(&symLink, LAB_SYMLINK_NAME);
    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_Device);
        g_Device = NULL;
        return status;
    }
    g_SymLinkCreated = TRUE;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = LabCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = LabCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LabDeviceControl;
    DriverObject->DriverUnload = LabUnload;

    DbgPrint("[LabDrv] loaded (benign PING/VERSION driver)\n");
    return STATUS_SUCCESS;
}
