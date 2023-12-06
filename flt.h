#pragma once

namespace flt {

	inline FLT_REGISTRATION FltRegistration = {};

	inline PFLT_FILTER FltFilter = {};

	inline PFLT_PORT FltServerPort = {};

	inline PFLT_PORT FltClientPort = {};

	auto SetRegistrKey(UNICODE_STRING* RegistryPath, wchar_t* Altitude) -> bool;

	auto PrepMiniFilter(PUNICODE_STRING RegistryPath, PWSTR Altitude) -> bool;

	auto RegisterFilter(DRIVER_OBJECT* DriverObject, PFLT_MESSAGE_NOTIFY fMessageNotifyCallback) -> NTSTATUS;

	NTSTATUS FilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS Flags);

	static NTSTATUS ConnectNotifyCallback(PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext, ULONG SizeOfContext, PVOID* ConnectionPortCookie);

	static VOID DisconnectNotifyCallback(PVOID ConnectionCookie);

	NTSTATUS MessageNotifyCallback(PVOID PortCookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnOutputBufferLength);
}