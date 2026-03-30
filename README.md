# myCopilot
# 我的副驾驶

## 截图

<p align="center">
  <img src="screenshots/ai-art.png" alt="生成星空截图" width="600">
</p>

<p align="center">
  <img src="screenshots/sidebar-usage.png" alt="侧边栏使用截图" width="600">
</p>


## 介绍
1. **背景**：在2024年11越18日，旧版Copilot in windows似乎被下架，新版Copilot界面美观，仅仅只是一个edge网页应用，缺失快捷键、靠边固定等功能。  
2. **目标**：myCopilot由C++和Webview2构成，在新版Copilot上添加了快捷键、后台运行、靠边固定、优化样式等多种功能。
## 计划
- [ ] 多语言支持
- [ ] 在C++层面实现和官方安卓版类似的启动Loading动画
- [ ] 实现和官方安卓版类似的网络错误画面
- [x] 程序启动时恢复上次的窗口位置大小和状态
- [x] 多次运行只会启动一个程序
- [x] 使用快捷键来隐藏和显示窗口
- [x] 窗口固定时自动推开其他窗口，使窗口不会被遮盖
- [x] 靠左固定、靠右固定、悬浮显示三种显示状态
- [x] 去除webview2加载初期的白屏

## 至开发者
* 该项目使用cmake+vcpkg+msvc构建，用到的第三方库以列出
* 配置文件在`./include/config.hpp`；**可修改开发者提供的选项**，编译后生效。
* 给网页注入的js在`./assets/script.js`；**可修改和增加Copilot网页的操作逻辑、功能和样式**，编译后生效。需要拥有前端html、js、css的基础知识，
* 头文件在 `./include`，源代码在`./src`；**可修改和增加程序在操作系统上的原生功能**，编译后生效。需要拥有win32api、c++的基础知识。

## 第三方库

[webview2](https://learn.microsoft.com/en-us/microsoft-edge/webview2/)
[fmt](https://github.com/fmtlib/fmt)
[simpleini](https://github.com/brofield/simpleini)