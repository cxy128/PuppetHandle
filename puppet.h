#pragma once

namespace puppet {

	auto OpenProcess(HANDLE ProcessId) -> HANDLE;

	static auto CopyEProcess(HANDLE ProcessId) -> PEPROCESS;
}
