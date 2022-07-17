#include <iostream>
#include <Windows.h>
#include <array>
#include <map>
#include "cheat_base/utils.h"
#include "cheat_base/thread.h"


struct Player
{
	float pos[3]{};				// 位置
	float skeletonHead[3]{};	// 头骨位置
	int camp = 0;				// 阵营
	int hp = 0;					// 血量
	int mirror = 0;				// 是否开镜
	int cmd = 0;				// 动作类型，32位，每一位代表一个动作，第一位是攻击
	float pitch = -1.f;			// 俯仰角
	float yaw = -1.f;			// 偏航角
};

struct ESPData
{
	static constexpr std::uint32_t s_redRectangle = RGB(255, 0, 0);
	static constexpr std::uint32_t s_blueRectangle = RGB(0, 0, 255);
	struct RenderData
	{
		RECT rect;
		std::uint32_t color;
		std::string textHP;
	};
	std::vector<RenderData> data;

	void clear()
	{
		data.clear();
	}
};


class CSGO : public Thread
{
public:
	CSGO();
	~CSGO();

	bool start();
	bool isRunning();

	bool& getAimboot() { return m_aimbot; }
	bool& getEsp() { return m_esp; }

private:

	virtual void threadRun() override;

	static LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT msgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool init();
	void run();
	void stop();

	bool createWindow();

	bool initGameModule();
	bool getGameProcess();
	void adjustWindow();

	// render
	void drawESP(HDC hdc);

	// game
	bool updateGameSignature();
	bool readGameGlobalData();
	void updateGame();
	void updateESP(const Player& player);
	void updateAimBot(const std::vector<Player*>& enemies);

private:

	static std::atomic<bool> m_stop;
	HWND m_mainWnd;
	HINSTANCE m_appInstance;

	int m_gameWindowWidth;
	int m_gameWindowHeight;
	HWND m_gameWnd;
	DWORD m_gamePid;
	HANDLE m_gameHandle;

	// game module
	ModuleData m_clientModule;
	ModuleData m_serverModule;
	ModuleData m_engineModule;

	// gloable address
	std::uintptr_t m_teamAddress;
	std::uintptr_t m_matrixAddress;
	std::uintptr_t m_viewAngleAddress;

	// matrix
	float m_viewProMatrix[16]{};

	// player
	Player* m_localPlayer;
	std::array<Player, 32> m_allPlayers;

	// esp
	ESPData m_espData;
	bool m_esp;

	// aimbot
	bool m_aimbot;
};


