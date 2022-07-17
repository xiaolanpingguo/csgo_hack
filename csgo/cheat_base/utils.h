#pragma once


#include <Windows.h>
#include <string>
#include <vector>


// dll module
struct ModuleData
{
	HANDLE handle;//ģ����
	std::wstring name;//ģ����
	DWORD address;//ģ�����ַ
	int size;//ģ���С
};

class Utils
{
public:
	// module
	static bool getModule(DWORD pid, const wchar_t* name, ModuleData* modudleData);

	// memory
	static std::size_t readMemory(HANDLE handle, std::uintptr_t address, void* buffer, int size);
	static std::size_t writeMemory(HANDLE handle, std::uintptr_t address, void* buffer, int size);
	static std::uintptr_t findPatternInMemory(HANDLE hPorcess, std::uintptr_t start, size_t size, const std::vector<std::uint8_t>& pattern) noexcept;

	// debug
	static void outputDebug(const char* pszFormat, ...);

	// math
	static bool worldToSceenDX(const float matrix[16], const float worldPos[3], int windoww, int windowh, int& screenx, int& screeny);
	static bool worldToSceenOpenGL(const float matrix[16], const float worldPos[3], int windoww, int windowh, int& screenx, int& screeny);
};

#define LOG_INFO(TEXT, ...) Utils::outputDebug(TEXT, __VA_ARGS__)
