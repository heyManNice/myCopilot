#include <windows.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <variant>
#include <map>
#include <tchar.h>

#include "WebView2.h"
#include <fmt/core.h>
#include <fmt/xchar.h>


#include "resource.hpp"
#include "webview.hpp"
#include "utils.hpp"
#include "config.hpp"

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE           hInst;
HWND                hMainWin;
WINDOWSATUSINFO     winInfo;
HMENU               hMenu;


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    HANDLE hMutex = CreateMutex(NULL, TRUE, _T("UniqueAppMutex"));

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWnd = FindWindow(_T("Copilot_UniqueWindowClass"),NULL);
        if (hWnd) {
            if (IsIconic(hWnd)) {
                ShowWindow(hWnd, SW_SHOW);
            } else {
                SetForegroundWindow(hWnd);
            }
        }
        CloseHandle(hMutex);
        return 0;
    }
    hInst = hInstance;
    
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	
    WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(RGB(250,240,231));;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _T("Copilot_UniqueWindowClass");
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	succeeded(
        RegisterClassEx(&wcex),
        ERROR,
        L"注册窗口类失败"
    );
    
	hMainWin = CreateWindowW(
		L"Copilot_UniqueWindowClass",
		Config::title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInst,
		NULL
	);

    succeeded(
        hMainWin!=nullptr,
        ERROR,
        L"创建窗口失败"
    );

    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hMainWin;
    nid.uID = ID_TRAY_EXIT;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));;
    strcpy_s(nid.szTip, "Copilot");
    Shell_NotifyIcon(NIM_ADD, &nid);

    hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPLEFT, L"靠左固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPRIGHT, L"靠右固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESUME, L"悬浮窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_HIDE, L"隐藏窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出程序");

    //设置快捷键
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_ID1, Config::fsModifiers, Config::vk),
        WARNING,
        L"设置全局快捷键失败"
    );

    std::wstring iniFile = Config::userDataFolder + L"/copilot.ini";
    if(loadWindowInfo(iniFile, winInfo)<0){
        loadRectTo(winInfo);
        winInfo.state = WIN_STATE::FLAOT;
    }
    copilotShow(winInfo.state);
    //创建webview2环境
	CreateCoreWebView2EnvironmentWithOptions(nullptr,Config::userDataFolder.c_str() , nullptr,pCreateEnvCallback.Get());



	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    
    //退出事件循环后为结束应用做收尾工作
    UnregisterHotKey(hMainWin, HOTKEY_ID1);
    saveWindowInfo(iniFile, winInfo);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyMenu(hMenu);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
	return (int)msg.wParam;
}

//事件循环回调函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_ACTIVATE:
        if(wParam != WA_INACTIVE){
            if(webviewController != nullptr){
                webviewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
            }
        }
        break;
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID1) {
            //相当于单击托盘图标
            SendMessage(hWnd, WM_TRAYICON, 0, WM_LBUTTONUP);
        }
        break;
    case WM_CLOSE:
        copilotShow(WIN_STATE::HIDE);
        return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
    case WM_SIZE:
        if(webviewController != nullptr){
            RECT bounds;
            GetClientRect(hMainWin, &bounds);
            webviewController->put_Bounds(bounds);
        }
        break;
    case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hMainWin);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_VERNEGANIMATION, pt.x, pt.y, 0, hMainWin, NULL);
            }
            if (lParam == WM_LBUTTONUP) {
                if(IsIconic(hMainWin)){
                    ShowWindow(hMainWin, SW_RESTORE);
                    SetForegroundWindow(hMainWin);
                }else
                if(IsWindowVisible(hMainWin)){
                    copilotShow(WIN_STATE::HIDE);
                }else{
                    copilotShow(winInfo.state);
                }
            }
            break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case ID_TRAY_EXIT:
                loadRectTo(winInfo);
                DestroyWindow(hMainWin);
                break;
            case ID_TRAY_KEEPLEFT:
                if(winInfo.state!=WIN_STATE::LEFT){
                    winInfo.state=WIN_STATE::LEFT;
                    copilotShow(WIN_STATE::LEFT);
                }
                break;
            case ID_TRAY_KEEPRIGHT:
                if(winInfo.state!=WIN_STATE::RIGHT){
                    winInfo.state=WIN_STATE::RIGHT;
                    copilotShow(WIN_STATE::RIGHT);
                }
                break;
            case ID_TRAY_RESUME:
                winInfo.state=WIN_STATE::FLAOT;
                copilotShow(WIN_STATE::FLAOT);
                break;
            case ID_TRAY_HIDE:
                copilotShow(WIN_STATE::HIDE);
                break;
        }
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}