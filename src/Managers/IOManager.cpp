#include <Managers/IOManager.h>
#include <Utils/Utils.h>
#include <Utils/MemAllocator.h>
#include <Windows.h>
#include <assert.h>

using namespace NoxEngine;

using NoxEngineUtils::Logger;
// using NoxEngine::PermanentMemAllocator;


PermResourceData IOManager::ReadEntireFilePerm(std::string filename) {

	HANDLE file = CreateFileA(
			(LPCSTR)filename.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);

	if(file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		LPSTR buf = NULL; 
		FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL,
				error,
				0,
				(LPSTR)&buf,
				0,
				NULL
				);
		LOG_DEBUG("Couldn't Open file (%s): %s", filename.c_str(), buf);
		LocalFree(buf);
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);

	if(size.QuadPart == 0) {
		LOG_DEBUG("File %s has zero size.", filename.c_str());
	}

	u8 *data = PermanentMemAllocator::Instance()->allocate(size.LowPart);

	DWORD readBytes = 0;
	ReadFile(file, (LPVOID)data, (DWORD)size.QuadPart, &readBytes, NULL);

	assert(size.QuadPart == readBytes );

	LOG_DEBUG("%s size: %d", filename.c_str(),  size.QuadPart);


	CloseHandle(file);
	PermResourceData m = {0};
	m.data = data;
	m.size = readBytes;
	return m;
}

TempResourceData IOManager::ReadEntireFileTemp(std::string filename) {

	HANDLE file = CreateFileA(
			(LPCSTR)filename.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);

	if(file == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		LPSTR buf = NULL; 
		FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL,
				error,
				0,
				(LPSTR)&buf,
				0,
				NULL
				);
		LOG_DEBUG("Couldn't Open file (%s): %s", filename.c_str(), buf);
		LocalFree(buf);
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);

	if(size.QuadPart == 0) {
		LOG_DEBUG("File %s has zero size.", filename.c_str());
	}

	LOG_DEBUG("%s size: %d", filename.c_str(),  size.QuadPart);

	DWORD readBytes = 0;

	u8 *data = StackMemAllocator::Instance()->allocate(i32(size.LowPart + 1));
	data[size.QuadPart] = '\0';

	ReadFile(file, (LPVOID)data, (DWORD)size.QuadPart, &readBytes, NULL);
	assert(size.QuadPart == readBytes );
	CloseHandle(file);
	return TempResourceData{data, (i32)readBytes};
}

std::string IOManager::PickFile(const char* filters) {

	std::string picked = "";

	// TODO(sharo): have multiple scratch memories

	u8 *file_name = ScratchMemAllocator::Instance()->allocate(1024);

	u8 dir[1024];
	GetCurrentDirectoryA(1024, (LPSTR)dir);

	HWND window_handle = (HWND)GetCurrentProcess();
	HINSTANCE instance = GetModuleHandle(NULL);
	OPENFILENAMEA open_file = {
		.lStructSize = sizeof(OPENFILENAMEA),
		.lpstrFilter = filters,
		.lpstrCustomFilter = NULL,
		.nMaxCustFilter = 0,
		.nFilterIndex = 1,
		.lpstrFile = (LPSTR)file_name,
		.nMaxFile = 1024,
		.lpstrInitialDir = (LPCSTR)dir,
		.lpstrTitle = "Selecting File...",
		.Flags = OFN_DONTADDTORECENT|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_NONETWORKBUTTON|OFN_NOCHANGEDIR,
	};

	if(!GetOpenFileNameA(&open_file)) {
		LOG_DEBUG("Failed to Open File Dialog: %x", CommDlgExtendedError() );
	}

	return std::string((char*)file_name);
}

