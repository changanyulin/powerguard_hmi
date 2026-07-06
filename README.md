# PowerGuard HMI

PowerGuard HMI 是一个基于 AWTK 和 C 语言实现的智能储能电源/BMS 人机界面 Demo。项目模拟便携式储能设备的触控屏，展示电池状态、单体电芯、电源告警、参数设置和本地配置保存等典型嵌入式 HMI 功能。

当前版本运行在 Windows PC 上，不依赖真实硬件。系统内部通过模拟器每 500 ms 生成一次电压、电流、温度、SOC 和告警状态数据，后续可以将模拟数据源替换为串口、CAN 或 MCU 通信模块。

## 功能

- 960 x 540 工业触控屏布局。
- 总览页展示 SOC 环形仪表、总电压、电流、温度、运行时间和工作模式。
- 电芯页展示 8 路单体电压、最高/最低电芯和均衡状态。
- 告警页展示活动告警和历史告警，支持告警确认和历史清除。
- 设置页支持温度阈值、电压阈值、电流阈值、屏幕亮度和工作模式配置。
- 配置保存到本地 `powerguard.ini`，应用重启后自动加载。
- 模拟器每 500 ms 刷新设备状态，趋势曲线同步更新。

## 技术亮点

- 采用 C 语言模块化设计，设备状态、数据模拟、告警管理、配置存储和 UI 控制相互独立。
- 使用 AWTK 原生控件构建工业触控屏界面，包括标签、按钮、编辑框、滑条、环形进度和自绘趋势图。
- 告警逻辑独立于界面刷新，支持活动状态、确认状态和历史记录管理。
- 设置参数具备边界限制和本地持久化能力，模拟嵌入式设备的非易失配置。
- 数据源设计可替换，后续可扩展为串口、CAN、MQTT 或真实 BMS/MCU 数据接入。

## 项目结构

```text
src/
  app.c              应用入口和定时刷新
  battery_model.*    电池状态、模式和告警数据结构
  simulator.*        本地模拟传感器数据
  alarm_manager.*    告警检测、确认和历史管理
  config_store.*     本地配置读写
  ui_controller.*    AWTK 界面创建、事件处理和数据刷新
SConstruct           SCons 编译脚本
```

## 编译

默认 AWTK 位于 `D:\dev\awtk`，Visual Studio Build Tools 位于 `D:\VS\BuildTools2022`。

```cmd
cd /d D:\dev\powerguard_hmi
call D:\VS\BuildTools2022\VC\Auxiliary\Build\vcvars64.bat
python -m SCons
```

运行：

```cmd
bin\powerguard_hmi.exe
```

如需指定 AWTK 路径：

```cmd
python -m SCons AWTK_ROOT=D:\dev\awtk
```

## 后续扩展

- 增加串口或 CAN 协议解析模块，对接真实 MCU/BMS 板卡。
- 增加更多故障类型和故障等级，例如过压、短路、绝缘异常等。
- 增加运行日志、数据导出和更完整的趋势曲线。
- 使用 AWStudio/Designer 维护 XML UI，提高界面迭代效率。
