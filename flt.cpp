#include <ntifs.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include "flt.h"

namespace flt {

#define MAX_PATH 0x260

	auto PrepMiniFilter(PUNICODE_STRING RegistryPath, PWSTR Altitude) -> bool {

		NTSTATUS Status = STATUS_SUCCESS;
		wchar_t* DriverName = NULL;
		wchar_t InstancesKey[MAX_PATH] = {0};
		wchar_t DefaultInstanceValueData[MAX_PATH] = {0};
		wchar_t MiniFilterInstanceKey[MAX_PATH] = {0};
		ULONG FlagsData = 0;

		DriverName = wcsrchr(RegistryPath->Buffer, L'\\');
		if (!MmIsAddressValid(DriverName)) {
			return false;
		}

		Status = RtlStringCbPrintfW(InstancesKey, sizeof(InstancesKey), L"%ws\\Instances", DriverName);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, InstancesKey);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlStringCbPrintfW(DefaultInstanceValueData, sizeof(DefaultInstanceValueData), L"%ws Instance", &DriverName[1]);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, InstancesKey, L"DefaultInstance", REG_SZ, DefaultInstanceValueData, (ULONG)(wcslen(DefaultInstanceValueData) * sizeof(WCHAR) + 2));
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlStringCbPrintfW(MiniFilterInstanceKey, sizeof(MiniFilterInstanceKey), L"%ws\\Instances%ws Instance", DriverName, DriverName);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, MiniFilterInstanceKey);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, MiniFilterInstanceKey, L"Altitude", REG_SZ, Altitude, (ULONG)(wcslen(Altitude) * sizeof(WCHAR) + 2));
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, MiniFilterInstanceKey, L"Flags", REG_DWORD, &FlagsData, sizeof ULONG);
		if (NT_ERROR(Status)) {
			return false;
		}

		return true;
	}

	auto SetRegistrKey(UNICODE_STRING* RegistryPath, wchar_t* Altitude) -> bool {

		NTSTATUS Status = STATUS_SUCCESS;
		wchar_t RegistryKey[0x260] = L"";
		wchar_t DefaultInstance[0x260] = L"";
		unsigned __int32 Flags = 0;

		auto DriverName = wcsrchr(RegistryPath->Buffer, L'\\');
		if (DriverName == nullptr) {
			return false;
		}

		Status = RtlStringCbPrintfW(RegistryKey, 0x260, L"%ws\\Instances", DriverName);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, RegistryKey);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlStringCbPrintfW(DefaultInstance, 0x260, L"%ws Instance", &DriverName[1]);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, RegistryKey, L"DefaultInstance", REG_SZ, DefaultInstance, 0x260);
		if (NT_ERROR(Status)) {
			return false;
		}

		RtlZeroMemory(RegistryKey, 0x260);

		Status = RtlStringCbPrintfW(RegistryKey, 0x260, L"%ws\\Instances%ws Instance", DriverName, DriverName);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, RegistryKey);
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, RegistryKey, L"Altitude", REG_SZ, Altitude, (ULONG)wcslen(Altitude));
		if (NT_ERROR(Status)) {
			return false;
		}

		Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, RegistryKey, L"Flags", REG_DWORD, &Flags, sizeof(unsigned __int32));
		if (NT_ERROR(Status)) {
			return false;
		}

		return true;
	}

	auto RegisterFilter(DRIVER_OBJECT* DriverObject, PFLT_MESSAGE_NOTIFY fMessageNotifyCallback) -> NTSTATUS {

		auto Status = STATUS_SUCCESS;

		FltRegistration.Size = sizeof FLT_REGISTRATION;
		FltRegistration.Version = FLT_REGISTRATION_VERSION;
		FltRegistration.FilterUnloadCallback = flt::FilterUnloadCallback;

		Status = FltRegisterFilter(DriverObject, &FltRegistration, &FltFilter);
		if (NT_ERROR(Status)) {
			return Status;
		}

		UNICODE_STRING PortName = {};
		RtlInitUnicodeString(&PortName, L"\\MyFilterPort");

		PSECURITY_DESCRIPTOR SecurityDescriptor = {};
		Status = FltBuildDefaultSecurityDescriptor(&SecurityDescriptor, FLT_PORT_ALL_ACCESS);
		if (NT_ERROR(Status)) {
			return Status;
		}

		OBJECT_ATTRIBUTES ObjectAttributes = {};
		InitializeObjectAttributes(&ObjectAttributes, &PortName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, nullptr, SecurityDescriptor);

		Status = FltCreateCommunicationPort(FltFilter, &FltServerPort, &ObjectAttributes, nullptr, ConnectNotifyCallback, DisconnectNotifyCallback, fMessageNotifyCallback, 1);
		if (NT_ERROR(Status)) {
			FltUnregisterFilter(FltFilter);
		}

		FltFreeSecurityDescriptor(SecurityDescriptor);

		return Status;
	}

	NTSTATUS FilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS Flags) {

		UNREFERENCED_PARAMETER(Flags);

		FltCloseCommunicationPort(FltServerPort);

		FltUnregisterFilter(FltFilter);

		return STATUS_SUCCESS;
	}

	static NTSTATUS ConnectNotifyCallback(PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext, ULONG SizeOfContext, PVOID* ConnectionPortCookie) {

		UNREFERENCED_PARAMETER(ServerPortCookie);
		UNREFERENCED_PARAMETER(ConnectionContext);
		UNREFERENCED_PARAMETER(SizeOfContext);
		UNREFERENCED_PARAMETER(ConnectionPortCookie);


		FltClientPort = ClientPort;

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "-> ConnectNotifyCallback ClientPort: %p \r\n", ClientPort);

		return STATUS_SUCCESS;
	}

	static VOID DisconnectNotifyCallback(PVOID ConnectionCookie) {

		UNREFERENCED_PARAMETER(ConnectionCookie);

		return FltCloseClientPort(FltFilter, &FltClientPort);
	}

	NTSTATUS MessageNotifyCallback(PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnOutputBufferLength) {

		UNREFERENCED_PARAMETER(PortCookie);
		UNREFERENCED_PARAMETER(InputBufferLength);
		UNREFERENCED_PARAMETER(OutputBufferLength);

		__try {

			*(__int8*)InputBuffer = *(__int8*)InputBuffer;
			*(__int8*)OutputBuffer = 0x00;

		} __except (EXCEPTION_EXECUTE_HANDLER) {

			return STATUS_SUCCESS;
		}

		*(__int8*)OutputBuffer = 0x4D;

		*ReturnOutputBufferLength = 0x01;

		return STATUS_SUCCESS;
	}
}