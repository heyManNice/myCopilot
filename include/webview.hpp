#pragma once
#include "WebView2.h"
#include "utils.hpp"
#include "config.hpp"
#include <vector>

//主窗口句柄
extern HWND hMainWin;

//webview窗口控制器的全局变量
wil::com_ptr<ICoreWebView2Controller> webviewController;
//webview核心功能控制器的全局变量
wil::com_ptr<ICoreWebView2> webview;

//页面导航完成回调函数
auto pNavigationCompletedCallback =
Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
[](ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
    auto script = readAppJsResource();
    webview->ExecuteScript(script.c_str(), nullptr);
    return S_OK;
});

auto pNewWindowRequestedCallback = 
Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>
([](ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args){
    wil::unique_cotaskmem_string uri;
    args->get_Uri(&uri);
    HINSTANCE result = ShellExecuteW(NULL, L"open", uri.get(), NULL, NULL, SW_SHOWNORMAL);

    //使用reinterpret_cast将HINSTANCE转换为uintptr_t
    //以避免指针截断
    succeeded(
        reinterpret_cast<uintptr_t>(result) > 32,
        WARNING,
        L"调用系统浏览器失败"
    );
    args->put_Handled(TRUE);
    return S_OK;
});


//创建webview控制器回调函数
auto pCreateControllerCallback = 
Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
([](HRESULT result, ICoreWebView2Controller* controller)->HRESULT{
    if (controller != nullptr) {
        webviewController = controller;
        webviewController->get_CoreWebView2(&webview);
    }

    wil::com_ptr<ICoreWebView2Settings> settings;
    webview->get_Settings(&settings);
    settings->put_AreDevToolsEnabled(Config::devToolsEnabled);
    settings->put_AreDefaultContextMenusEnabled(Config::contextMenusEnabled);
    settings->put_IsZoomControlEnabled(Config::zoomControlEnabled);
    settings->put_IsBuiltInErrorPageEnabled(TRUE);

    wil::com_ptr<ICoreWebView2Controller2> controller2;
    if (SUCCEEDED(controller->QueryInterface(IID_PPV_ARGS(&controller2)))) {
        COREWEBVIEW2_COLOR color = {0,0,0,0};
        controller2->put_DefaultBackgroundColor(color);
    }
    webview->Navigate(Config::index.c_str());
    webview->add_NavigationCompleted(pNavigationCompletedCallback.Get(), nullptr);
    webview->add_NewWindowRequested(pNewWindowRequestedCallback.Get(),nullptr);

    //修改webview2大小适应窗口
    RECT bounds;
    GetClientRect(hMainWin, &bounds);
    //webviewController->put_Bounds(bounds);
    return S_OK;
});

//创建webview环境回调函数
auto pCreateEnvCallback = 
Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
([](HRESULT result, ICoreWebView2Environment* env)->HRESULT{
    env->CreateCoreWebView2Controller(hMainWin, pCreateControllerCallback.Get());
	return S_OK;
});