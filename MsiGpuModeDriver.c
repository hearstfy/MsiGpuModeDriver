#include "MsiGpuModeDriver.h"
#include <wdf.h>
#include <ntddk.h>

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("GpuModeDriver: WdfDriverCreate failed with status 0x%x\n", status));
    }
    return status;
}

NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
    UNREFERENCED_PARAMETER(Driver);
    NTSTATUS status;
    WDFDEVICE device;
    PDEVICE_CONTEXT context;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\DosDevices\\GpuModeDriver");

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("GpuModeDriver: WdfDeviceCreate failed with status 0x%x\n", status));
        return status;
    }

    context = GetDeviceContext(device);
    context->Device = device;
    context->CurrentGpuMode = GPU_MODE_INTEGRATED;

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &context->Lock);
    if (!NT_SUCCESS(status)) {
        KdPrint(("GpuModeDriver: WdfSpinLockCreate failed with status 0x%x\n", status));
        return status;
    }

    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("GpuModeDriver: WdfDeviceCreateSymbolicLink failed with status 0x%x\n", status));
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    status = WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("GpuModeDriver: WdfIoQueueCreate failed with status 0x%x\n", status));
    }

    return status;
}

VOID EvtIoDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode) {
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT context = GetDeviceContext(device);
    ULONG mode;
    PVOID buffer;
    size_t bufferSize;

    switch (IoControlCode) {
    case IOCTL_SET_GPU_MODE:
        if (InputBufferLength < sizeof(ULONG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buffer, &bufferSize);
        if (!NT_SUCCESS(status)) {
            break;
        }
        mode = *(PULONG)buffer;
        if (mode != GPU_MODE_INTEGRATED && mode != GPU_MODE_DISCRETE) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        WdfSpinLockAcquire(context->Lock);
        context->CurrentGpuMode = mode;
        WdfSpinLockRelease(context->Lock);
        break;

    case IOCTL_GET_GPU_MODE:
        if (OutputBufferLength < sizeof(ULONG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &bufferSize);
        if (!NT_SUCCESS(status)) {
            break;
        }
        WdfSpinLockAcquire(context->Lock);
        *(PULONG)buffer = context->CurrentGpuMode;
        WdfSpinLockRelease(context->Lock);
        WdfRequestSetInformation(Request, sizeof(ULONG));
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestComplete(Request, status);
}