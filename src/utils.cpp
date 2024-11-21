#include <SimpleIni.h>
#include <utils.hpp>
#include <stdlib.h>
#include <shellapi.h>
#include "resource.hpp"
#include <tchar.h>
#include <windows.h>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include "config.hpp"




extern HWND hMainWin;
extern NOTIFYICONDATA nid;
extern Window_Info winInfo;

int LoadWindowInfo(std::wstring iniFile, Window_Info& info) {
	CSimpleIniA ini;
	SI_Error rc = ini.LoadFile(iniFile.c_str());
	if (rc < 0) { return -1; };
    info.WinRect.left = _ttoi(ini.GetValue("Window", "Left"));
    info.WinRect.top = _ttoi(ini.GetValue("Window", "Top"));
    info.WinRect.right = _ttoi(ini.GetValue("Window", "Right"));
    info.WinRect.bottom = _ttoi(ini.GetValue("Window", "Bottom"));
    info.state = static_cast<WIN_STATE>(_ttoi(ini.GetValue("Window", "State")));
    return 1;
}


int SaveWindowInfo(std::wstring iniFile, const Window_Info& info) {

    CSimpleIniA ini;
    TCHAR buffer[256];
    _itot(info.WinRect.left,buffer,10);
    ini.SetValue("Window", "Left", buffer);
    _itot(info.WinRect.top,buffer,10);
    ini.SetValue("Window", "Top", buffer);
    _itot(info.WinRect.right,buffer,10);
    ini.SetValue("Window", "Right", buffer);
    _itot(info.WinRect.bottom,buffer,10);
    ini.SetValue("Window", "Bottom", buffer);

    _itot((int)info.state,buffer,10);
    ini.SetValue("Window", "State", buffer);

    SI_Error rc = ini.SaveFile(iniFile.c_str());
	if (rc < 0) { /* handle error */ };

    return true;
}


void UnregisterAppBar(HWND hWnd) {
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hWnd;
    SHAppBarMessage(ABM_REMOVE, &abd);
}

void removeFrame(HWND hWnd){
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style &= ~WS_THICKFRAME & ~WS_CAPTION;
    SetWindowLongPtr(hWnd, GWL_STYLE, style);
}

void addFrame(HWND hWnd) {
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_CAPTION;
    SetWindowLongPtr(hWnd, GWL_STYLE, style);
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                 SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

std::wstring readAppJsResource() {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDI_SCRIPT), RT_RCDATA);
    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    DWORD resourceSize = SizeofResource(hModule, hResource);
    const char* pResourceData = static_cast<const char*>(LockResource(hLoadedResource));
    auto fileContent = std::vector<char>(pResourceData, pResourceData + resourceSize);
    return std::wstring(pResourceData, pResourceData + resourceSize);
}

void AddTrayIcon(HWND hWnd,HINSTANCE hInstance) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY_EXIT;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));;
    strcpy_s(nid.szTip, "Copilot");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPLEFT, L"靠左固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPRIGHT, L"靠右固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESUME, L"悬浮窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_HIDE, L"隐藏窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出程序");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}


void getWindowRect() {
    LONG_PTR style = GetWindowLongPtr(hMainWin, GWL_STYLE);
    if((style & WS_THICKFRAME) && (style & WS_CAPTION)){
        GetWindowRect(hMainWin, &winInfo.WinRect);
    }
}


void RegisterAppBar(HWND hWnd,BAR_EDGE edge) {
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hWnd;
    abd.uCallbackMessage = WM_ABDMSG;
    SHAppBarMessage(ABM_NEW, &abd);
    RECT workAreaRc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRc, 0);
    switch (edge)
    {
    case BAR_EDGE::LEFT:
        workAreaRc.right = workAreaRc.left + Config::dockedWindowWidth;
        abd.uEdge = ABE_LEFT;
        break;
    case BAR_EDGE::RIGHT:
        workAreaRc.left = workAreaRc.right - Config::dockedWindowWidth;
        abd.uEdge = ABE_RIGHT;
        break;
    default:
        break;
    }
    abd.rc = workAreaRc;
    SHAppBarMessage(ABM_QUERYPOS, &abd);
    SHAppBarMessage(ABM_SETPOS, &abd);

    int x = workAreaRc.left,y=workAreaRc.top,
        cx=workAreaRc.right - workAreaRc.left,
        cy=workAreaRc.bottom - workAreaRc.top;
    auto setPosCallback = [hWnd,x,y,cx,cy](){
        SetWindowPos(hWnd, NULL, x, y, 
        cx,
        cy, NULL);
        return;
    };
    //奇怪的bug：在当前情况下
    //windows似乎会干扰窗口位置的修改
    //即使APPBARDATA已被释放
    //窗口的位置有1/2的概率被移动到APPBARDATA之外
    //所以在靠边设置窗口位置之后
    //开新线程检查窗口是否在正确位置，否则重新设置
    //检查50次后仍然失败，警告退出重试
    ExecuteWithDelay([setPosCallback,x,y](){
        for(int i = 0;i<50;i++){
            RECT nowRc;
            GetWindowRect(hMainWin, &nowRc);
            if((nowRc.left==x)&&(nowRc.top==y)){
                ShowWindow(hMainWin, SW_SHOW);
                return;
            }
            setPosCallback();
            Sleep(50);
        }
        succeeded(
            FALSE,
            WARNING,
            L"设置窗口位置失败"
        );
    }, 1);
    
}

void reapplyWindowSettings(){
    switch (winInfo.state)
    {
    case WIN_STATE::LEFT:
        removeFrame(hMainWin);
        RegisterAppBar(hMainWin,BAR_EDGE::LEFT);
        break;
    case WIN_STATE::RIGHT:
        removeFrame(hMainWin);
        RegisterAppBar(hMainWin,BAR_EDGE::RIGHT);
        break;
    
    default:
        UnregisterAppBar(hMainWin);
        addFrame(hMainWin);
        SetWindowPos(hMainWin, NULL, winInfo.WinRect.left, winInfo.WinRect.top, 
                    winInfo.WinRect.right - winInfo.WinRect.left, winInfo.WinRect.bottom - winInfo.WinRect.top, 
                    SWP_NOZORDER);
        ShowWindow(hMainWin, SW_SHOW);
        break;
    }
}


void ExecuteWithDelay(std::function<void()> func, int delay) {
    auto threadFunc = [](void* param) -> DWORD {
        auto [func, delay] = *static_cast<std::pair<std::function<void()>, int>*>(param);
        Sleep(delay);
        func();
        delete static_cast<std::pair<std::function<void()>, int>*>(param);
        return 0;
    };
    auto* param = new std::pair<std::function<void()>, int>(func, delay);
    CreateThread(NULL, 0, threadFunc, param, 0, NULL);
}
void succeeded(BOOL result,int type,std::wstring msg){
    if(!result){
        MessageBoxW(NULL,msg.c_str(),fmt::format(L"错误码:{}",GetLastError()).c_str(),MB_OK|(type?MB_ICONWARNING:MB_ICONERROR));
        if(type==ERROR){
            PostQuitMessage(0);
        }
    }
}