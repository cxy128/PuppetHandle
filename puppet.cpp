#include <ntifs.h>
#include <intrin.h>
#include "puppet.h"

namespace puppet {

	static auto GetObHeaderCookie() {

		auto Process = (unsigned __int64)PsGetCurrentProcess();

		return ((unsigned __int8)((Process - 0x30) >> 0x08)) ^ (*(unsigned __int8*)(Process - 0x18)) ^ 7;
	}

	static auto CopyEProcess(HANDLE ProcessId) -> PEPROCESS {

		auto Status = STATUS_SUCCESS;
		PEPROCESS Process = {};
		Status = PsLookupProcessByProcessId(ProcessId, &Process);
		if (NT_ERROR(Status)) {
			return nullptr;
		}

		ObDereferenceObject(Process);

		auto PuppetCr3 = ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, PAGE_SIZE, 'Fake');
		if (PuppetCr3 == nullptr) {
			return nullptr;
		}

		KAPC_STATE Apc = {};
		KeStackAttachProcess(Process, &Apc);

		auto Cr3 = __readcr3() & ~0xfff;
		PHYSICAL_ADDRESS Cr3Pa = {};
		Cr3Pa.QuadPart = Cr3;
		auto Cr3Va = MmGetVirtualForPhysical(Cr3Pa);

		RtlCopyMemory(PuppetCr3, Cr3Va, PAGE_SIZE);

		KeUnstackDetachProcess(&Apc);

		auto PuppetProcess = (unsigned __int64)ExAllocatePool2(POOL_FLAG_NON_PAGED_EXECUTE, PAGE_SIZE * 2, 'Fake');
		if (!PuppetProcess) {
			return nullptr;
		}

		if (PuppetProcess & 0xfff) {
			PuppetProcess = (PuppetProcess + PAGE_SIZE) & ~0xfff;
		}

		auto ObHeaderCookie = GetObHeaderCookie();

		auto CopyProcess = PsGetCurrentProcess();
		auto CopyProcessOffset = (unsigned __int64)CopyProcess & 0xfff;

		RtlCopyMemory((void*)PuppetProcess, PAGE_ALIGN(CopyProcess), PAGE_SIZE);

		*(unsigned __int64*)(PuppetProcess + CopyProcessOffset + 0x28) = MmGetPhysicalAddress(PuppetCr3).QuadPart;
		*(unsigned __int8*)(PuppetProcess + CopyProcessOffset - 0x18) = (unsigned __int8)ObHeaderCookie ^ (unsigned __int8)0x07 ^ (unsigned __int8)((PuppetProcess + CopyProcessOffset - 0x30) >> 0x08);
		*(unsigned __int64*)(PuppetProcess + CopyProcessOffset + 0x7d8) = *(unsigned __int64*)((unsigned __int64)Process + 0x7d8);
		*(unsigned __int64*)(PuppetProcess + CopyProcessOffset + 0x550) = *(unsigned __int64*)((unsigned __int64)Process + 0x550);

		return (PEPROCESS)(PuppetProcess + CopyProcessOffset);
	}

	auto OpenProcess(HANDLE ProcessId) -> HANDLE {

		HANDLE ProcessHandle = nullptr;

		auto PuppetProcess = CopyEProcess(ProcessId);

		if (PuppetProcess != nullptr) {

			ObOpenObjectByPointer(PuppetProcess, 0, nullptr, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &ProcessHandle);
		}

		return ProcessHandle;
	}
}