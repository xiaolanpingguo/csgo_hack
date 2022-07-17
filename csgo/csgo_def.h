#pragma once


#include <vector>


static constexpr const wchar_t* CSGO_CLIENT_DLL_NAME = L"client.dll";
static constexpr const wchar_t* CSGO_SERVER_DLL_NAME = L"server.dll";
static constexpr const wchar_t* CSGO_ENGINE_DLL_NAME = L"engine.dll";

// ���������ַ
// �����룺EB ?? 8B4B ?? 83F9 FF 74 ?? 0FB7C1C1E0 ?? 05 ??
#define TEAMS_BASE				0x4DDB8FC

// �������飬ÿ������Ԫ�صļ����0x10���ֽ�
#define PLAYERS_ARRAY_OFFSET	0x10

// ����ṹ��
#define PLAYER_HP													0x100	// Ѫ��ƫ��,DWORD
#define PLAYER_POS													0xA0	// ����ƫ�ƣ�float
#define PLAYER_TEAM_FLAG											0xf4	// team��־λ��DWORD
#define PLAYER_MIRRO												0x31f4	// �Ƿ񿪾�, DWORD, ���һ������40����������10��Ĭ����0����90
#define PLAYER_COMMAND												0x300C	// ��������(��ǹ����������Ծ�ȣ�32λ��ÿһλ����һ�����ͣ���һλ�ǹ���)
#define PLAYER_PITCH												0x12C	// ������, float, ����-89��-����89��(�����޸�)
#define PLAYER_YAW													0x130	// ƫ���ǣ�float, �Ƕȣ�0-360��(�����޸�)
#define PLAYER_SKELETON_BASE										0x26a8	// ������ַƫ��
#define PLAYER_SKELETON_HEAD_X										0x18c	// ����ͷx����
#define PLAYER_SKELETON_HEAD_Y (PLAYER_SKELETON_HEAD_X + 0x10)				// ����ͷy����
#define PLAYER_SKELETON_HEAD_Z (PLAYER_SKELETON_HEAD_Y + 0x10)				// ����ͷz����


#define ENGINE_VISUAL_ANGLE_BASE		0x58CFDC	// �Ƕ�ƫ��base
#define ENGINE_VISUAL_VIEW_ANGLE		0x4d90		// pitch,yaw float[2]
#define ENGINE_CAMERA_MATRIX			0x4DCD214	// �����view-projection�����ַ
#define LOCAL_PLAYER					0x5284028



namespace GameSignature
{
	struct OffsetSignature
	{
		std::vector<std::uint8_t> signature;// p1{ 0xEB, '?', 0x8B, 0x4B, '?', 0x83, 0xF9, 0xFF, 0x74,'?', 0x0F, 0xB7, 0xC1 ,0xC1 ,0xE0 ,'?', 0x05, '?' };
		int offset;
		bool flag;
	};

	inline OffsetSignature g_teamBaseSignature
	{
		{ 0xEB, '?', 0x8B, 0x4B, '?', 0x83, 0xF9, 0xFF, 0x74,'?', 0x0F, 0xB7, 0xC1 ,0xC1 ,0xE0 ,'?', 0x05, '?' },
		17,
		true
	};
}