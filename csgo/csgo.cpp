#include "csgo.h"
#include "cheat_base/utils.h"
#include "csgo_def.h"
#include "cheat_base/process_mamager.h"
#include "cheat_base/imgui/imgui_impl_win32.h"
#include <Psapi.h>


static constexpr const wchar_t* APP_CLASS_NAME = L"cheat";
static constexpr const wchar_t* APP_TITLE_NAME = L"cheat";
static constexpr const std::wstring_view CSGO_WINDOW_TITLE_NAME1 = L"Counter-Strike: Global Offensive - Direct3D 9";
static constexpr const std::wstring_view CSGO_WINDOW_TITLE_NAME2 = L"Counter-Strike: Global Offensive";
static constexpr const std::wstring_view CSGO_PROCESS_NAME = L"csgo.exe";

std::atomic<bool> CSGO::m_stop = false;
CSGO::CSGO()
	: m_appInstance(nullptr)
	, m_gameWnd(nullptr)
	, m_mainWnd(nullptr)
	, m_gameHandle(nullptr)
	, m_gamePid(0)
	, m_esp(true)
	, m_aimbot(false)
{
	m_localPlayer = nullptr;
}

CSGO::~CSGO()
{
}

LRESULT WINAPI CSGO::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSGO* pThis;
	if (msg == WM_NCCREATE)
	{
		pThis = static_cast<CSGO*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

		SetLastError(0);
		if (!SetWindowLongPtr(hWnd, GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis)))
		{
			if (GetLastError() != 0)
			{
				return 0;
			}
		}
	}
	else 
	{
		pThis = reinterpret_cast<CSGO*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	return pThis->msgProc(hWnd, msg, wParam, lParam);
}

LRESULT CSGO::msgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		drawESP(hdc);

		EndPaint(hWnd, &ps);
	}
	break;

	case WM_CREATE:
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CSGO::start()
{
	threadStart();
	return true;
}

bool CSGO::init()
{
	if (!getGameProcess())
	{
		MessageBox(nullptr, TEXT("获取游戏窗口失败"), TEXT("提示"), MB_OK);
		return false;
	}

	if (!createWindow())
	{
		MessageBox(nullptr, TEXT("创建透明窗口失败"), TEXT("提示"), MB_OK);
		return false;
	}

	if (!initGameModule())
	{
		MessageBox(nullptr, TEXT("初始化游戏模块失败"), TEXT("提示"), MB_OK);
		return false;
	}

	return true;
}

bool CSGO::isRunning()
{
	return !m_stop;
}

bool CSGO::initGameModule()
{
	if (!Utils::getModule(m_gamePid, CSGO_CLIENT_DLL_NAME, &m_clientModule))
	{
		LOG_INFO("get module: %s faild!\n", CSGO_CLIENT_DLL_NAME);
		return false;
	}

	if (!Utils::getModule(m_gamePid, CSGO_SERVER_DLL_NAME, &m_serverModule))
	{
		LOG_INFO("get module: %s faild!\n", CSGO_SERVER_DLL_NAME);
		return false;
	}

	if (!Utils::getModule(m_gamePid, CSGO_ENGINE_DLL_NAME, &m_engineModule))
	{
		LOG_INFO("get module: %s faild!\n", CSGO_ENGINE_DLL_NAME);
		return false;
	}

	if (!updateGameSignature())
	{
		LOG_INFO("updateGameSignature faild!\n");
		return false;
	}

	if (!readGameGlobalData())
	{
		LOG_INFO("readGameGlobalData faild!\n");
		return false;
	}

	return true;
}

void CSGO::stop()
{
	m_stop = true;
}

bool CSGO::createWindow()
{
	HMODULE instance = GetModuleHandle(NULL);

	// wc.hbrBackground 0: 窗口背景色为黑色
	// wc.hbrBackground (HBRUSH)COLOR_WINDOW: 窗口背景色为默认的窗口的青色
	// wc.hbrBackground (HBRUSH)CreateSolidBrush(RGB(0, 0, 0)): 创建一个黑色的画刷
	// wc.hbrBackground (HBRUSH)GetStockObject(NULL_BRUSH)
	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = instance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszClassName = APP_CLASS_NAME;

	::RegisterClassEx(&wc);

	// dwStyle设置为WS_OVERLAPPEDWINDOW这个参数时，才可以使用CW_USEDEFAULT这个宏，否则的话，全部默认是0
	// window style: WS_OVERLAPPEDWINDOW: 重叠窗口，一般默认为这个，包含了标题，最小化等
	// window style: WS_POPUP: 可以认为就是无边框，无最小化等，只包含一个客户区
	// window style ex: WS_EX_LAYERED: 测试发现即使设置了WS_OVERLAPPEDWINDOW，这个值的效果也相当于是WS_POPUP
	// window style ex: WS_EX_TOPMOST: 永远保持窗口在顶层
	// window style ex: WS_EX_TRANSPARENT: 这个和WS_EX_LAYERED组合就可以实现点击到本窗口下面的窗口, 再配合WS_EX_TOPMOST
	// 即可实现点击窗口后，下层的窗口响应点击事件，同时本窗口依然在顶层
	HWND hwnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,//WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,                       // name of the window class
		APP_TITLE_NAME,                  // title of the window
		WS_POPUP,//WS_POPUP,                    // window style             
		400,                          // x-position of the window
		400,                          // y-position of the window
		400,                                    // width of the window
		400,                                    // height of the window
		NULL,                                   // we have no parent window, NULL
		NULL,                                   // we aren't using menus, NULL
		instance,                               // application handle
		this);                               // pass pointer to current object

	// 设置窗口透明，这里和创建窗口	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));有关
	// 相当于背景色是黑色，然后设置黑色将全部透明
	::SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	m_appInstance = (HINSTANCE)instance;
	m_mainWnd = hwnd;

	return true;
}

void CSGO::threadRun()
{
	if (!init())
	{
		stop();
		return;
	}

	run();
}

void CSGO::run()
{
	MSG msg = { 0 };
	while (!m_stop)
	{
		// we use PeekMessage instead of GetMessage here
		// because we should not block the thread at anywhere
		// except the engine execution driver module
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				m_stop = true;
			}
		}

		adjustWindow();
		updateGame();

		Sleep(1);
	}
}

void CSGO::updateGame()
{
	// get view projection matrix
	if (!Utils::readMemory(m_gameHandle, m_matrixAddress, m_viewProMatrix, sizeof(m_viewProMatrix)))
	{
		//LOG_INFO("matrix: %f, %f, %f, %f, \n"
		//	"%f, %f, %f, %f, \n"
		//	"%f, %f, %f, %f, \n"
		//	"%f, %f, %f, %f, \n",
		//	matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
		//	matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
		//	matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
		//	matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);
		return;
	}

	// get local player
	std::uintptr_t localPlayerAddress = 0;
	if (!Utils::readMemory(m_gameHandle, m_localPlayerAddress, &localPlayerAddress, sizeof(localPlayerAddress)) || localPlayerAddress == 0)
	{
		return;
	}

	std::vector<Player*> enemies;
	std::vector<Player*> allPlayers;
	for (size_t i = 0; i < m_allPlayers.size(); ++i)
	{
		Player& player = m_allPlayers[i];

		std::uintptr_t playerAddress;
		Utils::readMemory(m_gameHandle, m_teamAddress + i * PLAYERS_ARRAY_OFFSET, &playerAddress, sizeof(int));
		if (playerAddress == 0)
		{
			continue;
		}

		if (localPlayerAddress == playerAddress)
		{
			m_localPlayer = &player;
		}

		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_TEAM_FLAG, &player.team, sizeof(player.team));
		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_POS, &player.pos, sizeof(player.pos));
		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_MIRRO, &player.mirror, sizeof(player.mirror));
		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_COMMAND, &player.cmd, sizeof(player.cmd));
		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_HP, &player.hp, sizeof(player.hp));
		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_PITCH, &player.pitch, sizeof(player.pitch));
		Utils::readMemory(m_gameHandle, playerAddress + PLAYER_YAW, &player.yaw, sizeof(player.yaw));

		std::uintptr_t skeletonBase;
		if (Utils::readMemory(m_gameHandle, playerAddress + PLAYER_SKELETON_BASE, &skeletonBase, sizeof(skeletonBase)))
		{
			Utils::readMemory(m_gameHandle, skeletonBase + PLAYER_SKELETON_HEAD_X, &player.skeletonHead[0], sizeof(float));
			Utils::readMemory(m_gameHandle, skeletonBase + PLAYER_SKELETON_HEAD_Y, &player.skeletonHead[1], sizeof(float));
			Utils::readMemory(m_gameHandle, skeletonBase + PLAYER_SKELETON_HEAD_Z, &player.skeletonHead[2], sizeof(float));
		}

		allPlayers.emplace_back(&player);
	}

	if (m_localPlayer == nullptr)
	{
		return;
	}

	updateESP(allPlayers);
	updateAimBot(allPlayers);

	// trigger WM_PAINT message
	::InvalidateRect(m_mainWnd, nullptr, true);
}

bool CSGO::getGameProcess()
{
	HWND gameWnd = ::FindWindowW(NULL, CSGO_WINDOW_TITLE_NAME1.data());
	if (gameWnd == nullptr)
	{
		gameWnd = ::FindWindowW(NULL, CSGO_WINDOW_TITLE_NAME2.data());
		if (gameWnd == nullptr)
		{
			LOG_INFO("find game window faild!\n");
			return false;
		}
	}

	m_gamePid = ProcessManager::getPid(CSGO_PROCESS_NAME);
	if (m_gamePid == 0)
	{
		LOG_INFO("get process id faild!\n");
		return false;
	}

	m_gameHandle = ProcessManager::getProcessHandle(m_gamePid);
	if (m_gameHandle == nullptr)
	{
		LOG_INFO("get process handle faild!\n");
		return false;
	}

	m_gameWnd = gameWnd;
	return true;
}

void CSGO::adjustWindow()
{
	if (m_mainWnd == nullptr)
	{
		return;
	}

	// 保持本窗口在游戏窗口上
	RECT gameWindowRect = { 0 };
	::GetWindowRect(m_gameWnd, &gameWindowRect);

	int top = gameWindowRect.top;
	int left = gameWindowRect.left;
	int w = gameWindowRect.right - gameWindowRect.left;
	int h = gameWindowRect.bottom - gameWindowRect.top;

	m_gameWindowWidth = w;
	m_gameWindowHeight = h;

	DWORD dwStyle = ::GetWindowLong(m_gameWnd, GWL_STYLE);
	if (dwStyle & WS_BORDER)
	{
		top += 23;
		h -= 23;
	}

	MoveWindow(m_mainWnd, left, top, w, h, true);
}

void CSGO::drawESP(HDC hdc)
{
	if (m_espData.data.empty() || !m_esp)
	{
		return;
	}

	// 得到老画刷
	HBRUSH oldb = (HBRUSH)GetCurrentObject(hdc, OBJ_BRUSH);

	// 创建实心画刷
	HBRUSH redb = CreateSolidBrush(ESPData::s_redRectangle);
	HBRUSH blueb = CreateSolidBrush(ESPData::s_blueRectangle);

	// 开启当前绘图设备句柄的高级绘图模式，若显卡不支持，则本函数无效
	SetGraphicsMode(hdc, GM_ADVANCED);

	HFONT f1 = CreateFontA(
		10,// 字体的高度
		10,// 字体的宽度
		0,// 字体底线的斜度，单位是角度/10，比如我们希望字体的底线斜度是45度，则填写450
		0,// 字体本身的斜度，单位是 角度/10 ，比如我们希望字体本身的斜度45度，则填写450，必须开启高级绘图模式
		FW_NORMAL,// 字体粗细程度，一般FW_NORMAL
		FALSE,// 字体是否为斜体字
		FALSE,// 字体是否有下划线
		FALSE,// 字体是否有穿越线
		DEFAULT_CHARSET,// 字体使用的编码,DEFAULT_CHARSET表示当前的系统使用的编码
		OUT_DEFAULT_PRECIS,// 外观相关
		CLIP_DEFAULT_PRECIS,// 裁剪相关
		DEFAULT_QUALITY,// 质量相关
		DEFAULT_PITCH | FF_DONTCARE,// 不管
		"宋体");//外观名称，例如：黑体，宋体

	HFONT oldf = (HFONT)SelectObject(hdc, f1);

	// 设置字体背景为透明
	SetBkMode(hdc, TRANSPARENT);

	for (size_t i = 0; i < m_espData.data.size(); ++i)
	{
		ESPData::RenderData& espData = m_espData.data[i];

		if (espData.color == ESPData::s_redRectangle)
		{
			//设置字体颜色
			SetTextColor(hdc, ESPData::s_redRectangle);
			FrameRect(hdc, &espData.rect, redb);
		}
		else if (espData.color == ESPData::s_blueRectangle)
		{
			//设置字体颜色
			SetTextColor(hdc, ESPData::s_blueRectangle);
			FrameRect(hdc, &espData.rect, blueb);
		}

		int fontx = (int)(espData.rect.left + (espData.rect.right - espData.rect.left) * 0.5);
		int fonty = espData.rect.top;
		if (fontx != 0 && fonty != 0)
		{
			TextOutA(hdc, fontx, fonty, espData.textHP.c_str(), espData.textHP.length());
		}
	}

	DeleteObject(f1);
	SelectObject(hdc, oldf);

	DeleteObject(redb);
	DeleteObject(blueb);
	SelectObject(hdc, oldb);

	m_espData.clear();
}

bool CSGO::updateGameSignature()
{
	{
		std::uintptr_t addr = Utils::findPatternInMemory(m_gameHandle, m_clientModule.address, m_clientModule.size, GameSignature::g_teamBaseSignature.signature);
		if (addr == 0)
		{
			LOG_INFO("can't find g_teamBaseSignature!\n");
			return false;
		}

		std::uintptr_t targetAddr;
		std::uintptr_t readAddr = GameSignature::g_teamBaseSignature.flag ? addr + GameSignature::g_teamBaseSignature.offset : addr - GameSignature::g_teamBaseSignature.offset;
		Utils::readMemory(m_gameHandle, readAddr, &targetAddr, sizeof(targetAddr));

		m_teamAddress = targetAddr + 0x10; // 找的这个特征码不太好，需要额外加上0x10
		LOG_INFO("found m_teamAddress:%x, offset:%x!\n", m_teamAddress, m_teamAddress - m_clientModule.address);
	}
	{
		m_matrixAddress = m_clientModule.address + ENGINE_CAMERA_MATRIX;
	}

	return true;
}

bool CSGO::readGameGlobalData()
{
	std::uintptr_t angleBase = 0;
	float viewhAngle[2]{ 0.f };
	if (!Utils::readMemory(m_gameHandle, m_engineModule.address + ENGINE_VISUAL_ANGLE_BASE, &angleBase, sizeof(std::uintptr_t)))
	{
		assert(false);
		return false;
	}

	if (!Utils::readMemory(m_gameHandle, angleBase + ENGINE_VISUAL_VIEW_ANGLE, viewhAngle, sizeof(viewhAngle)))
	{
		assert(false);
		return false;
	}

	m_viewAngleAddress = angleBase + ENGINE_VISUAL_VIEW_ANGLE;
	m_localPlayerAddress = m_clientModule.address + LOCAL_PLAYER;

	LOG_INFO("CSGOGameManager::init success!,client.dll base: %x!, server.dll base:%x, engine.dll base:%x, team address:%x, matrix address:%x, "
		"m_viewAddress:%x, pitch:%f, yaw:%f\n",
		m_clientModule.address, m_serverModule.address, m_engineModule.address, 
		m_teamAddress, m_matrixAddress, m_viewAngleAddress, viewhAngle[0], viewhAngle[1]);

	return true;
}

void CSGO::updateESP(const std::vector<Player*>& allPlayers)
{
	if (!m_esp)
	{
		return;
	}

	for (size_t i = 0; i < allPlayers.size(); ++i)
	{
		Player& player = *allPlayers[i];

		if (&player == m_localPlayer
			|| player.hp <= 0
			|| m_localPlayer->team == player.team
			)
		{
			continue;
		}

		int screenSkeletonHeadx = 0, screenSkeletonHeady = 0;
		Utils::worldToSceenDX(m_viewProMatrix, player.skeletonHead, m_gameWindowWidth, m_gameWindowHeight, screenSkeletonHeadx, screenSkeletonHeady);

		int screenx = 0, screeny = 0;
		Utils::worldToSceenDX(m_viewProMatrix, player.pos, m_gameWindowWidth, m_gameWindowHeight, screenx, screeny);

		int playerHeight = screeny - screenSkeletonHeady;
		int playerWidth = int(playerHeight / 2); // 假设人物的模型宽度是身高的一半
		int extra = playerHeight / 6;  // 目前位置大概是嘴唇位置，增加少许高度把矩阵上边画在超过头的位置
		ESPData::RenderData esp;
		esp.rect.left = screenx - playerWidth / 2;
		esp.rect.top = screenSkeletonHeady - extra; //(screenSkeletonHeady - extra) < 0 ? 0 : screenSkeletonHeady - extra;
		esp.rect.right = esp.rect.left + playerWidth;
		esp.rect.bottom = esp.rect.top + playerHeight + extra;
		esp.color = m_localPlayer->team == player.team ? ESPData::s_blueRectangle : ESPData::s_redRectangle;
		esp.textHP = std::to_string(player.hp);

		m_espData.data.push_back(esp);
	}
}

void CSGO::updateAimBot(const std::vector<Player*>& allPlayers)
{
	if (!m_aimbot)
	{
		return;
	}

	// 没有攻击或者没开镜的时候，不触发自瞄
	if ((m_localPlayer->cmd & 1) == 0 && (m_localPlayer->mirror >= 90 || m_localPlayer->mirror <= 0 || m_localPlayer->hp <= 0))
	{
		return;
	}

	static constexpr float r2d = 57.295779513082f;

	struct Angles { float pitch{}; float yaw{}; };
	std::map<float, Angles> sortEnemies;
	for (size_t i = 0; i < allPlayers.size(); ++i)
	{
		Player& player = *allPlayers[i];

		if (player.team == m_localPlayer->team || player.hp <= 0)
		{
			continue;
		}

		// 获取目标和我的向量
		float x = player.skeletonHead[0] - m_localPlayer->pos[0];
		float y = player.skeletonHead[1] - m_localPlayer->pos[1];
		float z = player.skeletonHead[2] - m_localPlayer->pos[2] - 70.f;// 63是个优化的值，目的是定位到人物的摄像机位置
		float length = (float)(sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2)));


		Angles angle{ m_localPlayer->pitch, m_localPlayer->yaw };

		// 计算Yaw和Pitch角
		// 与目标保持在一个水平线，不需要计算yaw了，大于这个值才计算
		if (x > 0.01f || x < -0.01f)
		{
			// csgo的yaw是0-180度到-180度到0度为一个周期
			// 因此当向量0于第一和第四象限时，我们不做处理
			// 当处于第二象限(quadrant)时，得到的角度是负的，因此加上180度转换到第二象限
			// 同理第三象限亦是
			float yaw = atan(y / x) * r2d;
			if (x < 0.0f && y >= 0.0f)
			{
				yaw = yaw + 180.f;
			}
			else if (x < 0.0f && y < 0.0f)
			{
				yaw = yaw - 180.f;
			}
			angle.yaw = yaw;
		}

		// lenth太小，说明敌人就在面前，因此大于这个值才需要计算pitch
		if (length > 0.1f)
		{
			// csgo往上是0到-89度，因此要取反
			float pitch = asin(z / length) * r2d; // 180.0f / pi(3.1415926f)
			pitch = -pitch;
			angle.pitch = pitch;
		}

		sortEnemies.insert({ length ,angle });
	}

	if (sortEnemies.empty())
	{
		return;
	}

	for (const auto& pair : sortEnemies)
	{
		const Angles& angle = pair.second;

		float viewhAngle[2] = { angle.pitch, angle.yaw };
		if (viewhAngle[0] == m_localPlayer->pitch && viewhAngle[1] == m_localPlayer->yaw)
		{
			continue;
		}

		// 限制的自瞄角度是30度
		if (abs(viewhAngle[0] - m_localPlayer->pitch) > 30 || abs(viewhAngle[1] - m_localPlayer->yaw) > 30)
		{
			continue;
		}

		Utils::writeMemory(m_gameHandle, m_viewAngleAddress, &viewhAngle, sizeof(viewhAngle));
		return;
	}
}





