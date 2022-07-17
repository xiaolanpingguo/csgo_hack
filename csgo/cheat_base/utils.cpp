#include "utils.h"
#include <assert.h>
#include <vector>
#include <array> // patten
#include <tlhelp32.h> // module



bool Utils::getModule(DWORD pid, const wchar_t* name, ModuleData* modudleData)
{
	if (modudleData == nullptr || name == nullptr)
	{
		return false;
	}

	HANDLE hand = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hand == INVALID_HANDLE_VALUE) 
	{
		return false;
	}

	MODULEENTRY32 moduleInfo = {0};
	moduleInfo.dwSize = sizeof(moduleInfo);
	BOOL state = Module32First(hand, &moduleInfo);
	while (state) 
	{
		if (wcsncmp(moduleInfo.szModule, name, wcslen(name)) == 0)
		{		
			modudleData->size = moduleInfo.modBaseSize;
			modudleData->address = (DWORD)moduleInfo.modBaseAddr;
			modudleData->handle = moduleInfo.hModule;
			modudleData->name = moduleInfo.szModule;

			::CloseHandle(hand);
			return true;
		}

		state = Module32Next(hand, &moduleInfo);
	}

	return false;
}

void Utils::outputDebug(const char* pszFormat, ...)
{
	char buf[1024];
	va_list argList;
	va_start(argList, pszFormat);
	vsprintf_s(buf, pszFormat, argList);
	::OutputDebugStringA(buf);
	va_end(argList);
}

bool Utils::worldToSceenDX(const float matrix[16], const float worldPos[3], int windoww, int windowh, int& screenx, int& screeny)
{
	// TopLeftX��TopLeftYΪ��ƽ�������Ͻǵ�����㣬һ����˵��Ϊ0��WidthΪ��ƽ��Ŀ�ȣ�HeightΪ��ƽ��ĸ߶ȣ�MinDepthΪ�������С���(Z����),
	// MaxDepthΪ�����������(Z����), һ����˵MinDepthΪ0��MaxDepthΪ1.
						/*
							2/w,			0,					0,					0
							0,				-h/2,				0,					0
	[Xn, Yn, Zn, 1]	*		0,				0,					MaxDepth-MinDepth	0
							TopLeft+w/2,	TopLeftY + h/2		MinDepth,			1
						*/

	float worldPosx = worldPos[0]; 
	float worldPosy = worldPos[1];
	float worldPosz = worldPos[2];

	float clipCoordsx = worldPosx * matrix[0] + worldPosy * matrix[1] + worldPosz * matrix[2] + matrix[3] * 1;
	float clipCoordsy = worldPosx * matrix[4] + worldPosy * matrix[5] + worldPosz * matrix[6] + matrix[7] * 1;
	float clipCoordsz = worldPosx * matrix[8] + worldPosy * matrix[9] + worldPosz * matrix[10] + matrix[11] * 1;
	float clipCoordsw = worldPosx * matrix[12] + worldPosy * matrix[13] + worldPosz * matrix[14] + matrix[15] * 1;

	if (clipCoordsw < 0.01f)
	{
		return false;
	}

	// clipCoords -> NDC 
	float NDCx = clipCoordsx / clipCoordsw;
	float NDCy = clipCoordsy / clipCoordsw;

	// NDC -> screen coords
	screenx = (int)(windoww * 0.5 * (1.f + NDCx));
	screeny = (int)(windowh * 0.5 * (1.f - NDCy));

return true;
}

bool Utils::worldToSceenOpenGL(const float matrix[16], const float worldPos[3], int windoww, int windowh, int& screenx, int& screeny)
{
	// ����2D��Ļ��f��nһ��Ϊ0����˵����ж�Ϊ0
	/*
		2/w,	0,		0,			x + w/2
		0,		h/2,	0,			y + h/2
		0,		0,		(f-n)/2,	(f+n)/2			*  [Xn, Yn, Zn, 1]
		0,		0,		0,			1
	*/
	float worldPosx = worldPos[0];
	float worldPosy = worldPos[1];
	float worldPosz = worldPos[2];

	float clipCoordsx = worldPosx * matrix[0] + worldPosy * matrix[4] + worldPosz * matrix[8] + matrix[12] * 1;
	float clipCoordsy = worldPosx * matrix[1] + worldPosy * matrix[5] + worldPosz * matrix[9] + matrix[13] * 1;
	float clipCoordsz = worldPosx * matrix[2] + worldPosy * matrix[6] + worldPosz * matrix[10] + matrix[14] * 1;
	float clipCoordsw = worldPosx * matrix[3] + worldPosy * matrix[7] + worldPosz * matrix[11] + matrix[15] * 1;

	if (clipCoordsw < 0.01f)
	{
		return false;
	}

	// clipCoords -> NDC 
	float NDCx = clipCoordsx / clipCoordsw;
	float NDCy = clipCoordsy / clipCoordsw;

	// NDC -> screen coords
	screenx = (int)(windoww / 2 * NDCx + NDCx + windoww / 2);
	screeny = (int)(-windowh / 2 * NDCy + NDCy + windowh / 2); // reverse Y

	return true;
}

std::size_t Utils::readMemory(HANDLE handle, std::uintptr_t address, void* buffer, int size)
{
	SIZE_T read_zise;
	::ReadProcessMemory(handle, (LPCVOID)address, buffer, (SIZE_T)size, &read_zise);
	return (std::size_t)read_zise;
}

std::size_t Utils::writeMemory(HANDLE handle, std::uintptr_t address, void* buffer, int size)
{
	SIZE_T write_zise;
	::WriteProcessMemory(handle, (LPVOID)address, buffer, (SIZE_T)size, &write_zise);
	return (std::size_t)write_zise;
}

std::uintptr_t Utils::findPatternInMemory(HANDLE hPorcess, std::uintptr_t start, size_t size, const std::vector<std::uint8_t>& pattern) noexcept
{
	std::uint8_t buffer[0x1000] = { 0 };
	std::uintptr_t endAddr = start + size;
	size_t patternLen = pattern.size();
	for (std::uintptr_t readAddr = start; readAddr <= endAddr - 0x1000; readAddr += (0x1000 - patternLen))
	{
		memset(buffer, 0, sizeof(buffer));
		BOOL r = ::ReadProcessMemory(hPorcess, (LPCVOID)readAddr, buffer, sizeof(buffer), nullptr);
		if (r == 0)
		{
			continue;
		}

		for (size_t i = 0; i < 0x1000 - patternLen; ++i)
		{
			bool found = true;
			for (size_t j = 0, k = i; j < patternLen; ++j, ++k)
			{
				if (pattern[j] == '?')
				{
					continue;
				}

				if (pattern[j] != buffer[k])
				{
					found = false;
					break;
				}
			}

			if (found)
			{
				return std::uintptr_t(readAddr + i);
			}
		}
	}

	return 0;
}