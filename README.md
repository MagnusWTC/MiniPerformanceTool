# MiniPerformanceTool

轻量 Windows 桌面性能监测工具，使用 C++17、Qt Widgets 和 CMake。

## 运行

双击 `dist/MiniPerformanceTool.exe`。部署时请复制整个 `dist` 目录，不能只复制 exe。

- 主窗口显示系统 CPU 与物理内存使用率，每秒刷新；CPU 首次采样约等待 1 秒。
- CPU、内存告警阈值分别设置，点击“保存设置”应用并保存；默认均为 80%。
- “开机自启动”勾选后立即生效，取消勾选立即移除；该设置仅对当前 Windows 用户生效。程序移动后请重新勾选以更新启动路径。
- 任一指标严格超过阈值，悬浮窗平滑变红；两项都不超过阈值时变绿。
- 左键拖动悬浮窗，靠近屏幕边缘时自动吸附，拖动露出的边缘区域即可恢复圆形；双击打开主窗口；右键可隐藏悬浮窗。
- 悬浮窗采用 104×104 的球形外观，显示 CPU 和内存的整数百分比，主窗口保留一位小数。
- 阈值输入框支持直接输入数值及键盘上下箭头调整。
- 主窗口点击 X 后隐藏到托盘；点击托盘图标恢复；托盘右键菜单选择“退出程序”结束。
- 阈值和悬浮窗位置由 QSettings 保存在当前 Windows 用户配置中。

## 构建、测试、打包

需要 Visual Studio 2022 C++ 桌面开发工具、Windows SDK、Qt 6 的 MSVC x64 版本（含 Qt Test）。

在工程目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Deploy
```

脚本默认使用 `C:\Qt\6.8.3\msvc2022_64`，可通过 `-QtRoot` 指定安装目录。自动初始化 MSVC 环境、配置 Ninja Release、构建、运行 CTest，并通过 windeployqt 生成 `dist`。`-SkipTests` 可关闭测试构建。

也可在 Qt Creator 中打开 `CMakeLists.txt`，选择 Qt 6 MSVC x64 Kit。

## 模块

| 模块 | 职责 |
| --- | --- |
| MetricsProvider / WindowsMetricsProvider | 系统数据采集接口与 Windows 实现 |
| MetricsController | 采样定时和数据分发 |
| MainWindow | 性能概览与阈值配置 |
| FloatingWindow / ThemeAnimation | 悬浮窗绘制、拖动及状态动画 |
| TrayController | 托盘和主窗口显示控制 |
| SettingsManager | 阈值、位置持久化 |

目前面向 Windows 10/11；非 Windows 可构建，但尚未实现对应的性能采集器。没有后台网络服务和联网采集。

任务及验收状态见 TASKS.md。
