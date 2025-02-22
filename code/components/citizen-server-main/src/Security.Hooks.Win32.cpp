#include "StdInc.h"

#include <ntstatus.h>
#include <winternl.h>

#include "MinHook.h"

static wchar_t workingDirectory[MAX_PATH];

static NTSTATUS NTAPI NtCreateUserProcessHook()
{
	trace("DEBUG: Blocked process creation\n");
	return STATUS_ACCESS_DENIED;
}

using NtCreateFile_t = NTSTATUS(NTAPI*)(
PHANDLE FileHandle,
ACCESS_MASK DesiredAccess,
POBJECT_ATTRIBUTES ObjectAttributes,
PIO_STATUS_BLOCK IoStatusBlock,
PLARGE_INTEGER AllocationSize,
ULONG FileAttributes,
ULONG ShareAccess,
ULONG CreateDisposition,
ULONG CreateOptions,
PVOID EaBuffer,
ULONG EaLength);

static NtCreateFile_t origNtCreateFileHook = nullptr;

static constexpr std::wstring_view PREFIX = L"\\??\\";

#define FILE_WRITE_DATA              0x0002
#define FILE_APPEND_DATA             0x0004
#define FILE_WRITE_EA                0x0010
#define FILE_WRITE_ATTRIBUTES        0x0100
#define FILE_WRITE_ACCESS_MASK  (FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES)

static NTSTATUS NTAPI NtCreateFileHook(
PHANDLE FileHandle,
ACCESS_MASK DesiredAccess,
POBJECT_ATTRIBUTES ObjectAttributes,
PIO_STATUS_BLOCK IoStatusBlock,
PLARGE_INTEGER AllocationSize,
ULONG FileAttributes,
ULONG ShareAccess,
ULONG CreateDisposition,
ULONG CreateOptions,
PVOID EaBuffer,
ULONG EaLength
)
{
	if (ObjectAttributes && ObjectAttributes->ObjectName)
	{
		const UNICODE_STRING* filePath = ObjectAttributes->ObjectName;
		std::wstring filePathStr(filePath->Buffer, filePath->Length / sizeof(wchar_t));

		if (filePathStr.rfind(PREFIX, 0) == 0)
		{
			filePathStr = filePathStr.substr(PREFIX.length());
		}

		if (filePathStr.rfind(workingDirectory, 0) != 0)
		{
			if ((DesiredAccess & FILE_WRITE_ACCESS_MASK) != 0)
			{
				const std::string filePathStrConv(filePathStr.begin(), filePathStr.end());
				trace("DEBUG: Blocked write access to %s\n", filePathStrConv.c_str());
				return STATUS_ACCESS_DENIED;
			}
		}
	}

	return origNtCreateFileHook(
	FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize,
	FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength
	);
}

static InitFunction initFunction([]()
{
	GetCurrentDirectoryW(MAX_PATH, workingDirectory);

	MH_Initialize();

	MH_CreateHookApi(L"ntdll.dll", "NtCreateUserProcess", NtCreateUserProcessHook, nullptr);

	MH_CreateHookApi(L"ntdll.dll", "NtCreateFile", &NtCreateFileHook, reinterpret_cast<LPVOID*>(&origNtCreateFileHook));

	MH_EnableHook(MH_ALL_HOOKS);
}, INT_MIN);
