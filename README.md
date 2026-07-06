# PowerGuard HMI

PowerGuard HMI 是一个基于 AWTK 的桌面端工业触控屏 Demo，用来模拟便携式储能电源 /
BMS 管理界面。项目定位为嵌入式和单片机方向的作品展示：它包含产品化 HMI、模拟数据源、
告警逻辑、配置持久化，以及清晰的 C 语言模块拆分。

## 功能

- 960 x 540 工业触控屏布局。
- 总览页展示 SOC 环形仪表、总电压、电流、温度、运行时间和工作模式。
- 电芯页展示 8 路单体电压和均衡状态。
- 告警页展示活动/历史告警，支持确认和清除历史。
- 设置页支持阈值、亮度、工作模式配置，并保存到本地。
- 模拟器每 500 ms 刷新一次数据。

## 编译

默认 AWTK 位于 `D:\dev\awtk`，Visual Studio Build Tools 位于
`D:\VS\BuildTools2022`。

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

## 面试讲解要点

- `battery_model` 负责设备状态和用户阈值。
- `simulator` 目前生成模拟数据，后续可替换为串口、MQTT 或 MCU 数据适配层。
- `alarm_manager` 将告警检测从 UI 刷新中拆离。
- `ui_controller` 负责把设备状态映射到 AWTK 控件和事件。
- `powerguard.ini` 展示基础的本地配置持久化能力。
