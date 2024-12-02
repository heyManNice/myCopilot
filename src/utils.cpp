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
extern WINDOWSATUSINFO winInfo;

int loadWindowInfo(std::wstring iniFile, WINDOWSATUSINFO& info) {
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


int saveWindowInfo(std::wstring iniFile, const WINDOWSATUSINFO& info) {

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


std::wstring readAppJsResource() {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDI_SCRIPT), RT_RCDATA);
    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    DWORD resourceSize = SizeofResource(hModule, hResource);
    const char* pResourceData = static_cast<const char*>(LockResource(hLoadedResource));
    auto fileContent = std::vector<char>(pResourceData, pResourceData + resourceSize);
    return std::wstring(pResourceData, pResourceData + resourceSize);
}

void loadRectTo(WINDOWSATUSINFO &info) {
    if(!IsIconic(hMainWin)&&IsWindowVisible(hMainWin)&&winInfo.state==WIN_STATE::FLAOT){
        GetWindowRect(hMainWin, &info.WinRect);
    }
}

void setTimeout(std::function<void()> func, int delay) {
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
        std::wstring errorMessage = L"错误码: " + std::to_wstring(GetLastError());
        MessageBoxW(NULL,msg.c_str(),errorMessage.c_str(),MB_OK|(type?MB_ICONWARNING:MB_ICONERROR));
        if(type==ERROR){
            PostQuitMessage(0);
        }
    }
}

void copilotShow(WIN_STATE state){
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    abd.uCallbackMessage = WM_ABDMSG;
    abd.hWnd = hMainWin;
    SHAppBarMessage(ABM_REMOVE, &abd);
    switch (state)
    {
    case WIN_STATE::HIDE:
        ShowWindow(hMainWin, SW_HIDE);
        break;
    case WIN_STATE::FLAOT:{
            int x = winInfo.WinRect.left;
            int y = winInfo.WinRect.top;
            int w = winInfo.WinRect.right-winInfo.WinRect.left;
            int h = winInfo.WinRect.bottom-winInfo.WinRect.top;
            SetWindowLongPtr(hMainWin,GWL_STYLE,WS_OVERLAPPEDWINDOW);
            SetWindowLongPtr(hMainWin,GWL_EXSTYLE,WS_EX_OVERLAPPEDWINDOW);
            SetWindowPos(hMainWin, HWND_NOTOPMOST, x, y, w, h,SWP_FRAMECHANGED);
            ShowWindow(hMainWin, SW_SHOW);
            SetForegroundWindow(hMainWin);
            break;
        }

    default:{
        HMONITOR hMonitor = MonitorFromWindow(hMainWin, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(hMonitor, &monitorInfo);
        RECT workAreaRc = monitorInfo.rcWork;
        //SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRc, 0);
        if(state==WIN_STATE::LEFT){
            workAreaRc.right = workAreaRc.left + Config::dockedWindowWidth;
            abd.uEdge = ABE_LEFT;
        }else if(state==WIN_STATE::RIGHT){
            workAreaRc.left = workAreaRc.right - Config::dockedWindowWidth;
            abd.uEdge = ABE_RIGHT;
        }
        abd.rc = workAreaRc;
        SHAppBarMessage(ABM_NEW, &abd);
        SHAppBarMessage(ABM_QUERYPOS, &abd);
        SHAppBarMessage(ABM_SETPOS, &abd);
        SetWindowLongPtr(hMainWin,GWL_STYLE,WS_POPUP);
        SetWindowLongPtr(hMainWin,GWL_EXSTYLE,WS_EX_TOOLWINDOW);
        int x = abd.rc.left;
        int y = abd.rc.top;
        int w = abd.rc.right-abd.rc.left;
        int h = abd.rc.bottom-abd.rc.top;
        SetWindowPos(hMainWin, HWND_TOPMOST, x, y, w, h, SWP_FRAMECHANGED);
        if(state==WIN_STATE::LEFT){
            AnimateWindow(hMainWin, 100,AW_HOR_POSITIVE);
        }else if(state==WIN_STATE::RIGHT){
            AnimateWindow(hMainWin, 100,AW_HOR_NEGATIVE);
        }
        SetForegroundWindow(hMainWin);
        break;
        }
    }
}