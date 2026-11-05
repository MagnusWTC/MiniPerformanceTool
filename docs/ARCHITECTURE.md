# MiniPerformanceTool 项目实现说明

本文按当前源码说明项目的启动方式、各个自定义类型的职责、性能数据和设置的流向，以及两个窗口的控件实现方式。项目使用 C++17、Qt Widgets 和 CMake，当前性能采集实现面向 Windows。

## 1. 项目结构

```text
src/
  main.cpp                       程序入口与对象组装
  MetricsProvider.h              采样结果数据结构和采集接口
  WindowsMetricsProvider.*       Windows 性能指标采集
  MetricsController.*            定时采样并发出数据
  MainWindow.*                   主窗口和阈值设置界面
  FloatingWindow.*               桌面悬浮窗、自绘和鼠标交互
  ThemeAnimation.*               悬浮窗告警颜色动画
  TrayController.*               系统托盘及窗口显示控制
  SettingsManager.*              阈值和悬浮窗位置持久化
tests/
  MetricsControllerTest.cpp      采样控制器测试
  WidgetBehaviorTest.cpp          悬浮窗和主题动画测试
```

模块关系可以概括为：

```text
WindowsMetricsProvider
          │ sample()
          ▼
MetricsController ──metricsUpdated(snapshot)──┬──> MainWindow
                                               └──> FloatingWindow
MainWindow ──thresholdsChanged(cpu, memory)───────> FloatingWindow
            └──thresholdsChanged(cpu, memory)────> SettingsManager
```

`main.cpp` 创建这些对象并建立 Qt 信号槽连接。各模块通过值类型 `MetricsSnapshot` 和信号槽协作，不需要由采集器直接依赖窗口。

## 2. 启动和运行时序

程序从 `main.cpp` 开始：

1. 创建 `QApplication`，设置 Fusion 风格、全局字体、应用名和组织名。设置 `setQuitOnLastWindowClosed(false)`，使最后一个普通窗口关闭时应用仍可驻留托盘。
2. 创建 `MetricsController`、`FloatingWindow`、`MainWindow` 和 `TrayController`。它们是 `main()` 的局部对象，事件循环结束后按栈对象生命周期析构。
3. 连接采样更新信号到两个窗口，连接主窗口阈值信号到悬浮窗和设置存储，连接两个窗口的显示状态信号与槽。
4. 从 `SettingsManager` 读取 CPU、内存阈值并设置主窗口和悬浮窗状态。
5. 显示两个窗口并启动采样控制器，最后进入 `app.exec()` Qt 事件循环。

控制器默认周期为 1000 ms。`start()` 除启动定时器外还立即调用一次 `sampleNow()`，所以启动时马上有一个初始快照；这并不意味着首个 CPU 数据已经有效。Windows CPU 使用率需要前后两次系统计数器作差，首个样本只建立基准，`valid` 为 `false`。一个周期后才通常能得到有效 CPU 使用率。由于快照只有一个整体 `valid` 标志，首个样本即使读到了内存数据，两个界面仍按无效样本处理。

## 3. 项目自定义类型及职责

### 3.1 `MetricsSnapshot`

定义在 `src/MetricsProvider.h` 的轻量数据结构，是采样层与 UI 层之间的统一数据契约：

- `cpuUsagePercent`：CPU 使用百分比。
- `memoryUsagePercent`：物理内存使用百分比。
- `timestampMs`：采样时间，Unix 毫秒时间戳。
- `valid`：该快照是否可供界面作为有效采样使用。

结构通过 `Q_DECLARE_METATYPE` 声明，并在控制器构造时用 `qRegisterMetaType` 注册，以便作为 Qt 信号参数传递，也能被 `QSignalSpy` 等测试设施读取。

### 3.2 `MetricsProvider`

定义在 `src/MetricsProvider.h` 的抽象接口，只有虚析构函数和 `sample()`。它隔离“如何从操作系统读指标”和“何时采样、把结果发给谁”。控制器接收 `std::unique_ptr<MetricsProvider>`，因此可替换实现，也便于测试注入假采集器。

### 3.3 `WindowsMetricsProvider`

定义在 `src/WindowsMetricsProvider.h/.cpp`，实现 Windows 指标采集：

- 用 `GetSystemTimes()` 读取 idle、kernel、user 的累计 FILETIME 计数。
- 把 FILETIME 合并为 64 位值，保留上一次计数作为基线。后续样本计算 `idleDelta` 和 `totalDelta`，CPU 忙碌比例为 `1 - idleDelta / totalDelta`，并限制在 0 到 100 之间。
- 用 `GlobalMemoryStatusEx()` 获取总物理内存与可用物理内存，计算 `(1 - available / total) * 100`。
- `timestampMs` 每次调用都会设置。CPU 首次基线尚不存在时，CPU 区间读数不可用；只有 CPU 区间和内存读取都成功时，整体 `valid` 才为真。
- 非 Windows 构建仍能通过编译，但当前返回无效快照；接口留出了以后增加其他平台 provider 的位置。

### 3.4 `MetricsController`

定义在 `src/MetricsController.h/.cpp`，负责采样调度和数据分发，而不负责计算系统指标或绘制界面：

- 默认构造函数注入 `WindowsMetricsProvider`；另一个构造函数接受任意 `MetricsProvider`。
- 内部 `QTimer` 默认每 1000 ms 触发 `sampleNow()`。
- `setIntervalMs()` 把周期限制在 100 至 60000 ms。
- `start()` 启动定时器并立即采样；重复调用不会重复启动定时器，但仍会立即采一笔。
- `stop()` 停止后续定时采样。
- `sampleNow()` 调用 provider，并通过 `metricsUpdated(snapshot)` 广播结果。

### 3.5 `MainWindow`

定义在 `src/MainWindow.h/.cpp`，负责主界面性能概览、阈值编辑、状态说明以及关闭行为：

- `setMetrics()` 接收快照，检查有效标志和数值是否有限，再将数值限制在 0 至 100。有效时更新数值文本和进度条；无效时显示 `-- %` 并清空进度条。
- 进度条范围为 0 至 1000，将百分比乘 10 后显示，从而保留 0.1% 的显示精度；主窗口文字也显示一位小数。
- CPU 和内存阈值由 `QDoubleSpinBox` 编辑，范围 1.0 至 100.0、精度一位小数。输入不会在每次键入时即时应用，点击“保存设置”才触发 `applyThresholds()`。
- `setThresholds()` 用于从持久化值初始化输入框和已应用阈值；恢复默认按钮将两项设为 80 并立即应用。
- `refreshStatus()` 对有效样本分别比较 CPU、内存与阈值，比较条件是严格大于；无效样本显示等待状态。
- 悬浮窗复选框发出 `floatingVisibilityRequested`。反向同步可见状态时通过 `QSignalBlocker` 暂停复选框信号，避免同步行为再次触发显示命令。
- 有系统托盘时，关闭事件被忽略并隐藏主窗口；托盘不可用时接受关闭并退出应用。托盘控制器也安装事件过滤器，在托盘可用时先行拦截关闭事件并隐藏窗口。

### 3.6 `FloatingWindow`

定义在 `src/FloatingWindow.h/.cpp`，是一个专门绘制的悬浮性能窗口，兼任自绘、指标显示、阈值告警和鼠标交互：

- 继承 `QWidget`，窗口标志设为 `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint`，启用透明背景，固定尺寸 104×104。
- `setMetrics()` 保存完整快照，更新告警状态，并启动数值动画。当前值到新值之间用 0 到 1 的进度做线性插值；新样本到达时先停止旧动画，从当前显示值重新开始。
- `updateAlert()` 仅在样本有效时判断告警。CPU 或内存任一项严格高于各自阈值即进入告警状态；无效采样不报警。
- `paintEvent()` 使用 `QPainter` 和径向渐变绘制圆形底色、柔和外圈、CPU/内存标签与整数百分比。无效快照显示 `--`。绘制不依赖独立子控件，因此圆形外观和透明区域可精确控制。
- 左键按下记录鼠标相对窗口偏移，移动时据此移动整个窗口；释放时检查屏幕边缘并吸附，随后立即保存坐标。
- 靠近边缘 24 px 内时吸附，仅将 18 px 窗口区域留在屏幕内，便于抓取露出的部分。窗口初始化时会校正越界位置，且能恢复仍有可操作露出区域的既有贴边位置。
- 左键双击发出 `openMainWindowRequested`；右键菜单包含打开主界面和隐藏悬浮窗。
- `showEvent()` / `hideEvent()` 发出 `visibilityChanged`，用于同步主界面复选框和托盘菜单勾选状态。析构时再次保存当前位置，鼠标释放时也已即时保存。

### 3.7 `ThemeAnimation`

定义在 `src/ThemeAnimation.h/.cpp`，只负责悬浮窗的告警主题色渐变：

- 正常色为绿色系 `(52, 211, 153)`，告警色为红色系 `(248, 87, 105)`。
- 内部使用 `QVariantAnimation`，时长 650 ms，缓动曲线为 `InOutCubic`。
- 动画颜色变化时发出 `colorChanged`；悬浮窗连接该信号并调用 `update()` 重新绘制。
- 状态变化会从当前颜色开始并停止旧动画，所以告警反转不会跳回端点。

需要区分两种动画：`ThemeAnimation` 动画的是绿色/红色主题；`FloatingWindow::m_valuesAnimation` 动画的是 CPU 和内存显示数字，时长 550 ms，使用 `OutCubic`。

### 3.8 `TrayController`

定义在 `src/TrayController.h/.cpp`，管理系统托盘图标、菜单和主窗口恢复：

- 通过 `QPainter` 在内存中绘制托盘图标，不依赖图标资源文件。
- 创建“打开主界面”“显示悬浮窗”勾选项和“退出程序”菜单项。
- 托盘单击或双击调用 `showMainWindow()`；该方法显示、置前并激活主窗口。
- 菜单项直接控制悬浮窗显示，并与悬浮窗发出的可见状态同步勾选状态。
- 对主窗口安装事件过滤器：托盘可用时截获关闭事件并隐藏窗口。退出菜单先移除过滤器再调用 `QApplication::quit()`。
- 析构时移除事件过滤器、隐藏托盘，并解除托盘菜单关联后释放菜单。

### 3.9 `SettingsManager`

定义在 `src/SettingsManager.h/.cpp`，以静态方法封装 `QSettings` 访问。当前使用组织名和应用名 `MiniPerformanceTool`，保存：

- `thresholds/cpu`，默认 80.0。
- `thresholds/memory`，默认 80.0。
- `floatingWindow/position`，默认 `(24, 24)`。
- 开机自启动状态不写入应用配置，而是映射到当前用户的
  `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run` 注册表项。

它不持有长期设置对象；每个方法创建一个 `QSettings` 实例并读取或写入对应键。Windows 上 Qt 通常将这类设置映射到当前用户配置存储。

开机自启动使用值名 `MiniPerformanceTool`，值为带双引号的当前可执行文件绝对路径。勾选时写入该值并立即 `sync()`；取消勾选时删除该值。该方式只影响当前 Windows 用户，不需要管理员权限，也不安装 Windows 服务。程序移动位置后，注册表中的旧路径会失效，需要在界面中重新切换一次“开机自启动”。

### 3.10 测试辅助类型

测试文件也定义了少量仅用于测试的类型，不参与应用运行：

- `FakeProvider`（`MetricsControllerTest.cpp`）：实现 `MetricsProvider`，按序返回预置快照并统计调用次数，用于验证控制器转发、即时采样、定时采样、停止与周期限制。
- `MetricsControllerTest`：Qt Test 测试对象，覆盖初始无效与后续有效快照转发、定时器启停、周期边界。
- `WidgetBehaviorTest`：Qt Test 测试对象，覆盖主题动画及反向过渡、阈值边界、无效样本不告警、双击信号和两种悬浮窗渲染状态。
- 测试匿名命名空间内的 `snapshot()`、`saveRender()` 等是构造测试数据和保存渲染图的辅助函数。

## 4. 数据流详解

### 4.1 性能采样到界面

```text
QTimer timeout (每秒)
  -> MetricsController::sampleNow()
  -> MetricsProvider::sample()
  -> WindowsMetricsProvider 读取 Windows 累计计数器和内存状态
  -> 返回 MetricsSnapshot
  -> MetricsController::metricsUpdated(snapshot)
       ├─ MainWindow::setMetrics(snapshot)
       │    ├─ 校验 valid / isfinite
       │    ├─ 更新数值、进度条
       │    └─ 按当前已应用阈值刷新状态文字
       └─ FloatingWindow::setMetrics(snapshot)
            ├─ 保存快照并判断告警
            ├─ 更新主题色动画
            ├─ 缓动显示数值
            └─ update() 请求 paintEvent 重绘
```

这条链路是单向广播：provider 不知道窗口存在；controller 只依赖 provider 接口；两个窗口各自决定如何呈现同一份快照。

### 4.2 阈值变更

```text
用户编辑输入框
  -> 点击“保存设置”或“恢复默认”
  -> MainWindow::applyThresholds()
  -> thresholdsChanged(cpu, memory)
       ├─ FloatingWindow::setThresholds() 立即更新告警颜色状态
       └─ SettingsManager 写入 CPU / 内存阈值
```

主窗口的 `m_appliedCpuThreshold` 和 `m_appliedMemoryThreshold` 代表当前生效值。单纯编辑输入框不会改变告警判断，点击保存才会更新当前生效值并写入配置。启动时 `main.cpp` 从设置读取两项值并初始化主窗口；悬浮窗构造时也自行读取阈值，因此两个视图启动时均能取得保存值。

### 4.3 显示状态与托盘

复选框改变时，主窗口发出 `floatingVisibilityRequested` 并驱动悬浮窗 `setVisible()`。悬浮窗 show/hide 事件发出 `visibilityChanged`，再同步主窗口复选框和托盘菜单勾选项。托盘菜单的“显示悬浮窗”也可直接切换同一个悬浮窗。

主窗口关闭与程序退出是两件事：托盘可用时关闭主窗口只隐藏它；托盘菜单的“退出程序”才结束事件循环。托盘不可用时，主窗口关闭会退出应用。

### 4.4 悬浮窗位置

悬浮窗创建时读取保存的位置，随后根据所在屏幕可用区域执行位置校正。用户拖动后在释放时吸附到最近的可用边缘（若足够接近），并立即保存位置；析构时再保存一次。坐标保存的是窗口左上角位置。

## 5. 自定义控件和界面构建思路

主界面使用标准 Qt Widgets 拼装，而不是把视觉结构拆成很多自定义 QWidget 类：

- `QMainWindow` 承载中央 `QWidget`。
- `QVBoxLayout`、`QHBoxLayout`、`QFormLayout` 负责纵向页面、指标并排和阈值表单布局。
- `QFrame` 作为指标区域和设置区域的容器；文件内匿名命名空间的 `metricCard()` 和 `thresholdInput()` 是构造辅助函数，不是继承 Qt 控件的 class。
- `QLabel`、`QProgressBar`、`QDoubleSpinBox`、`QPushButton`、`QCheckBox` 等标准控件承担显示和输入。
- `setObjectName()` 配合窗口级 Qt Style Sheet 控制颜色、字体、边框和状态样式。

这种做法适合当前规模：布局、控件属性和信号连接都集中在 `MainWindow` 构造函数中，局部差异由小型构造函数复用。悬浮窗的形状、渐变和文本排版需要精确绘制，因而由 `FloatingWindow::paintEvent()` 使用 `QPainter` 自绘；鼠标行为则通过重写 Qt 的鼠标事件实现。告警色动画独立封装在 `ThemeAnimation`，避免把插值逻辑混进绘制代码。

## 6. 为什么没有 UI 文件

项目没有 `.ui` 文件，是因为界面是直接用 C++ 运行时创建的：在 `MainWindow` 构造函数中逐个创建控件、设置属性、放入布局并建立信号槽；悬浮窗则通过 `paintEvent()` 绘制图形。也就是说，界面定义的“源文件”就是 `MainWindow.cpp`、`FloatingWindow.cpp` 等代码，而不是 Qt Designer 的 XML 表单。

`CMakeLists.txt` 中确实设置了 `CMAKE_AUTOUIC ON`，但这只是启用 CMake 的自动 UI 编译扫描。当前源码没有 `.ui` 文件，也没有包含 `ui_*.h` 生成头文件，因此不会产生 Designer 生成的 UI 类。启用 AUTOUIC 不代表项目必须使用 Qt Designer。

代码创建界面的优点是可以直接表达动态布局、状态逻辑和自绘行为，改动可与信号槽逻辑一同审查；代价是页面结构和样式没有 Designer 的可视化编辑能力，复杂页面的构造函数也可能变长。若未来主窗口继续扩展，可以把静态页面迁移为 `.ui`，但悬浮窗的自绘部分仍通常保留在 C++。

## 7. 自动化测试覆盖

CTest 注册两个测试可执行文件：

- `MetricsControllerTest` 使用 `FakeProvider` 隔离 Windows API，验证采样快照是否按原样广播、启动时立即采样、定时更新、停止后不再调用 provider，以及采样周期限制。
- `WidgetBehaviorTest` 验证主题色到达红绿端点及反向动画连续性、CPU/内存阈值严格大于边界、无效快照不报警、左键双击请求打开主界面，并渲染正常和告警两种悬浮窗图像。

当前自动化测试重点在采样控制器和悬浮窗行为。主窗口布局、托盘在不同桌面环境中的行为，以及多显示器拔插和不同 DPI 组合，仍需结合实际 Windows 桌面环境验证；`TASKS.md` 也将这些长时间运行和多显示器场景列为尚未完成的人工验收项。

## 8. 阅读源码建议

建议按以下顺序阅读，能先理解对象如何连接，再深入各个实现：

1. `src/main.cpp`：入口、对象生命周期和信号槽连接。
2. `src/MetricsProvider.h`、`src/WindowsMetricsProvider.cpp`：数据契约和 Windows 采集算法。
3. `src/MetricsController.h/.cpp`：采样时钟与 provider 注入。
4. `src/MainWindow.cpp`：主窗口运行时布局、设置应用和状态计算。
5. `src/FloatingWindow.cpp`、`src/ThemeAnimation.cpp`：绘制、输入事件、边缘吸附和动画。
6. `src/TrayController.cpp`、`src/SettingsManager.cpp`：驻留行为和配置持久化。
7. `tests/`：从测试用例确认边界行为和模块契约。
