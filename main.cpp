#include <ntifs.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include "flt.h"
#include "puppet.h"

NTSTATUS fMessageNotifyCallback(PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnOutputBufferLength) {

	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	__try {

		*(__int8*)InputBuffer = *(__int8*)InputBuffer;
		*(__int8*)OutputBuffer = 0x00;

	} __except (EXCEPTION_EXECUTE_HANDLER) {

		return STATUS_SUCCESS;
	}

	auto ProcessId = *(HANDLE*)InputBuffer;
	*(HANDLE*)OutputBuffer = puppet::OpenProcess(ProcessId);

	*ReturnOutputBufferLength = sizeof HANDLE;

	return STATUS_SUCCESS;
}

auto DriverUnload(DRIVER_OBJECT* DriverObject) {

	UNREFERENCED_PARAMETER(DriverObject);
}

extern "C" auto DriverEntry(DRIVER_OBJECT * DriverObject, UNICODE_STRING * RegistryPath) -> NTSTATUS {

	wchar_t Altitude[] = L"321656";

	DriverObject->DriverUnload = DriverUnload;

	if (flt::PrepMiniFilter(RegistryPath, Altitude)) {

		return flt::RegisterFilter(DriverObject, fMessageNotifyCallback);

	} else return STATUS_ACCESS_DENIED;
}