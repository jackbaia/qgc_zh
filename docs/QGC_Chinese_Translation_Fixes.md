# QGC 中文飞控术语与参数说明修正记录

## 扫描范围

本次重点扫描并校对了以下文件和目录类型：

- `translations/qgc_source_zh_CN.ts`
- `translations/qgc_json_zh_CN.ts`
- `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml`
- `src/FirmwarePlugin/**/ParameterFactMetaData*`
- `src/AutoPilotPlugins/**/*`
- `src/VehicleSetup/**/*`
- `src/FactSystem/**/*`
- `src/QmlControls/**/*`
- `src/AnalyzeView/**/*`
- `resources/**/*`

重点搜索词包括：`手臂`、`胳膊`、`解除武装`、`武装`、`最低俯仰高度`、`俯仰高度`、`态度`、`使命`、`故障安全`、`离板`、`家庭位置`、`栅栏`、`收音机`、`力量`、`被指导`、`集会点`、`车辆`、`Arm switch channel`、`Minimum pitch angle`、`Failsafe`、`Offboard`、`Home Position`、`Radio`、`Vehicle` 等。

## 修改文件

- `translations/qgc_source_zh_CN.ts`
- `translations/qgc_json_zh_CN.ts`
- `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml`
- `src/QmlControls/MainStatusIndicator.qml`
- `docs/ChineseTerminology_QGC.md`

## 参数 Metadata 文件

- `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml`

本轮对 PX4 参数 metadata 进行了定点重写和术语清理，参数名、单位、枚举 code、MAVLink message name 均未翻译。重点修正 `short_desc` / `long_desc` 中的飞控语境错误，例如 `RC_MAP_ARM_SW`、`FW_P_LIM_MIN`、`VT_PITCH_MIN`、`VT_LND_PITCH_MIN`、`VT_FW_QC_P`、`VT_FW_QC_R`、`VT_QC_ALT_LOSS`、`VT_QC_T_ALT_LOSS`、`VTO_LOITER_ALT`、`VTQ_DISARM_TRIG` 等。

## 修正数量

- 本轮可计数术语与描述替换：1,333 处。
- 其中 PX4 参数 metadata 定点/批量修正：1,329 处。
- `qgc_source_zh_CN.ts` 本轮重点残留修正：4 处。
- 已验证 `.ts` 与 `.xml` 格式有效，未破坏 `%1`、`%2`、`%n`、HTML 标签。

## 典型错误修正表

| 英文原文 | 原中文 | 新中文 | 文件路径 | 修改原因 |
|---|---|---|---|---|
| Arm switch channel | 手臂开关通道 | 解锁开关通道 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Arm` 在飞控语境是解锁，不是手臂。 |
| Channel used for arming/disarming | 使用它通过开关而不是默认的油门杆来布防/未解锁 | 用于通过遥控器开关解锁或上锁飞行器 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 重写为 RC 解锁/上锁通道说明。 |
| Arm switch is a momentary button | 手臂开关是一个瞬时按钮 | 解锁开关为瞬时按钮 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 避免把 `Arm` 直译成手臂。 |
| RC arm/disarm command duration | RC输入布/未解锁命令持续时间 | RC 解锁/上锁命令保持时间 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `disarm` 应为上锁，且 RC 输入需保留遥控语境。 |
| Stick gesture arming | 启用布防/未解锁摇杆手势 | 启用解锁/上锁摇杆手势 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 统一飞控解锁/上锁术语。 |
| Minimum pitch angle | 最小桨距角设定值 | 最小俯仰角设定值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Pitch` 在姿态/固定翼限制中是俯仰角。 |
| Minimum pitch in hover | 悬停时的最小俯仰角 | 悬停时的最小俯仰角 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 确认并保留正确表达。 |
| Landing minimum pitch | 悬停着陆时的最小俯仰角 | 悬停着陆时的最小俯仰角 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 确认 `pitch` 为俯仰角，不是高度。 |
| VTOL pitch min description | 减小桨距角 | 减小俯仰角 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 避免误解为螺旋桨桨距。 |
| Quadchute pitch threshold | 四溜槽最大俯仰阈值 | Quadchute 最大俯仰角阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | Quadchute 是垂起保护逻辑，不是“滑槽”。 |
| Quadchute roll threshold | 四滑槽最大侧倾阈值 | Quadchute 最大横滚角阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Roll` 在姿态语境应为横滚角。 |
| Quadchute pitch trigger | 四槽触发的绝对音调阈值 | 触发 Quadchute 的绝对俯仰角阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Pitch` 不是音调。 |
| Quadchute roll trigger | 四槽触发的绝对横滚阈值 | 触发 Quadchute 的绝对横滚角阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 补足角度语义。 |
| Quadchute altitude loss | 四滑道非指令下降阈值 | Quadchute 非指令下降阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正机器直译。 |
| Quadchute transition altitude loss | 四滑道过渡高度损失阈值 | Quadchute 转换高度损失阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 还原 VTOL 转换保护语境。 |
| VTOL transition | 前部过渡 | 前向转换 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | VTOL `front transition` 是前向转换。 |
| Home altitude | 相对于家的高度 | 相对于 Home 点盘旋的高度 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Home` 是返航/Home 点，不是家庭。 |
| Disarmed trigger behavior | PX4 手臂上的触发行为 | PX4 上锁时的触发行为 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Disarmed` 是上锁状态。 |
| Flight controller disarmed | 飞行控制器解除武装时 | 飞控处于上锁状态时 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正军事化直译。 |
| Failsafe PWM threshold | 故障安全通道 PWM 阈值 | 失效保护通道 PWM 阈值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Failsafe` 标准飞控表达为失效保护。 |
| Failsafe channel mapping | 故障安全通道映射 | 失效保护通道映射 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 统一术语。 |
| Failsafe output value | 故障安全值 | 失效保护值 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 输出失效保护值不是“安全故障”。 |
| Vehicle | 车辆 | 飞行器 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | QGC/PX4 常见飞行器语境不应显示车辆。 |
| Attitude | 态度 | 姿态 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 飞行姿态术语修正。 |
| Mission | 使命 | 任务 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 航线任务语境。 |
| Radio | 收音机 | 遥控器 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | QGC 中 `Radio` 多指遥控器/遥控链路。 |
| Flow board | 流量板 | 光流板 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 传感器语境为 optical flow。 |
| Accelerometer | 加速器 | 加速度计 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | IMU 传感器术语修正。 |
| Roll rate | 滚动速率 | 横滚角速度 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 控制器 `rate` 为角速度。 |
| Yaw rate | 偏航率 | 偏航角速度 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 控制器 `rate` 为角速度。 |
| Angular rate | 角速率 | 角速度 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 飞控控制术语统一。 |
| GeoFence transfer failed | GeoFence（地理栅栏） 传输失败 | 地理围栏传输失败 | `translations/qgc_source_zh_CN.ts` | `Geofence` 应为地理围栏。 |
| Follow vehicle parameters | 跟随模式的车辆参数设置不支持 | 跟随模式所需的飞行器参数当前配置不受支持 | `translations/qgc_source_zh_CN.ts` | 改为工程说明而非机器句式。 |
| Current sensor offset | 若车辆没有电流通过而传感器读数过高 | 如果飞行器在几乎无电流通过时仍报告较高电流读数 | `translations/qgc_source_zh_CN.ts` | 补足电流传感器偏置语境。 |
| Internal Vehicle request error | 呼叫车辆_requestNextMission项目 | 调用 Vehicle::_requestNextMissionItem | `translations/qgc_source_zh_CN.ts` | 保留 C++ 方法名，避免误译。 |
| Ubuntu serial permissions | 删除调制解调器管理器 | 卸载 modemmanager | `translations/qgc_source_zh_CN.ts` | 保留包名与 HTML 格式。 |

## 验证

- 已解析 `qgc_source_zh_CN.ts`、`qgc_json_zh_CN.ts`、`PX4ParameterFactMetaData.xml`，XML 格式有效。
- 已检查 `%1`、`%2`、`%n` 占位符与 HTML 标签，没有发现破坏。
- 已确认重点中文错误词在 `qgc_source_zh_CN.ts`、`qgc_json_zh_CN.ts`、`PX4ParameterFactMetaData.xml` 中不再出现。
- 已使用 `lrelease` 生成测试 `.qm` 文件，翻译文件可被 Qt 工具正常处理。

## COM_ARM_AUTH / COM_PARACHUTE 定向修复

本轮继续修复 PX4 参数 metadata 中的机器翻译问题，重点补充 `Arm/Arming` 在飞控参数语境下应译为“解锁”，并修正 MAVLink 降落伞健康检查说明。

| 英文原文/参数名 | 原中文 | 新中文 | 文件路径 | 修改原因 |
|---|---|---|---|---|
| `COM_ARM_AUTH_REQ` / `arm authorization` | 请求“臂”授权 / ARM 授权 | 请求解锁授权 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `arm authorization` 在飞控参数语境中是解锁授权，不能直译为“臂”相关授权。 |
| `COM_ARM_AUTH_METHOD` / `COM_ARM_AUTH_MET` | ARM授权方式 | 解锁授权方式 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 统一 COM_ARM_AUTH 参数组术语，将 `Arm` 修正为“解锁”。 |
| `One step arm` | 一步 + “臂” | 单步解锁 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `arm` 表示飞行器解锁，机器直译为“臂”不符合飞控语境。 |
| `Two step arm` | 两步 + “臂” | 两步解锁 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正参数枚举说明中的解锁流程术语。 |
| `arm command` / `arm vehicle` | 第 1/2 个“臂”相关命令 / “臂”飞行器 | 第一次解锁命令 / 第二次解锁命令 / 解锁飞行器 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 将授权流程说明改写为清晰的飞控中文表达。 |
| `arming authorization` | ARM 授权 | 解锁授权 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Arming` 在 PX4/QGC 语境中应译为“解锁”。 |
| `COM_PARACHUTE` / `Expect and require a healthy MAVLink parachute system` | “期望”加“需要”的直译句 | 要求存在并检测到健康的 MAVLink 降落伞系统 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 避免英文直译腔，明确这是系统健康检查要求。 |
| `healthy MAVLink parachute system` | 健康的 MAVLink 降落伞系统 | 健康的 MAVLink 降落伞系统 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 保留 MAVLink 专名，同时补足“存在并检测到”的参数语义。 |

## 第二轮定向修复

本轮继续面向 PX4 参数 metadata、参数标题、短描述和长描述做飞控语境校对，重点覆盖 `Kill`、`Stick`、`Gesture`、`Arm/Disarm`、`Board`、`Expo` 等容易被机器直译的词。

| 英文原文/参数名 | 原中文 | 新中文 | 文件路径 | 修改原因 |
|---|---|---|---|---|
| `MAN_KILL_GEST_T` | 击杀棒手势的触发时间 | 紧急停机摇杆动作触发时间 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Kill` 在飞控语境是紧急停机，`stick gesture` 是摇杆动作。 |
| Kill stick gesture trigger time | 击杀棒手势的触发时间 | 紧急停机摇杆动作触发时间 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 禁止“击杀/棒手势”机器直译。 |
| Kill stick gesture description | 同时按住左摇杆到左下角和右摇杆到右下角直到手势单向终止执行器的超时时间 | 同时将左摇杆保持在左下角、右摇杆保持在右下角，达到该时间后触发执行器紧急停机 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 重写为清晰的遥控器摇杆动作说明。 |
| Arm switch is a momentary button | 解锁开关为瞬时按钮 | 解锁开关为自复位按钮 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 遥控器开关语境中 `momentary button` 优先译为自复位按钮。 |
| Momentary button hold description | 按住瞬时按钮达到 COM_RC_ARM_HYST | 按住自复位按钮达到 COM_RC_ARM_HYST | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 保持参数名不变，修正按钮类型表达。 |
| Board rotation / `SENS_BOARD_ROT` | 董事会轮换 | 飞控板安装方向 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Board` 是飞控板，不是董事会。 |
| FMU board rotation | 该参数定义 FMU 板相对于平台的旋转 | 定义 FMU 飞控板相对于机体坐标系的安装方向 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 补足机体坐标系语境，避免“平台”含糊。 |
| Geofence termination | 违反围栏时杀死飞行器 | 违反围栏时将触发紧急停机 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `kill` 行为应解释为紧急停机/飞行终止。 |
| Stick gesture | 摇杆手势 | 摇杆动作 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 遥控器语境中 gesture 更自然表达为动作。 |
| Arm switch channel description | 油门摇杆手势 | 油门摇杆动作 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 统一 RC 解锁/上锁动作术语。 |
| Flight mode momentary button | 使用瞬时按钮指定多个通道 | 使用自复位按钮指定多个通道 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | RC 按键切换模式语境使用自复位按钮。 |
| `FW_POS_STK_CONF` | 自定义棒配置 | 自定义摇杆配置 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Stick` 是遥控器摇杆，不是棒。 |
| `MC_ACRO_SUPEXPO` description | 提供直观的棒感 | 提供较自然的摇杆手感 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正遥控器手感表达。 |
| `MC_ACRO_SUPEXPOY` description | 提供直观的棒感 | 提供较自然的摇杆手感 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正遥控器手感表达。 |
| `MC_ACRO_EXPO` | Acro 模式横滚、俯仰曝光系数 | Acro 模式横滚、俯仰指数曲线系数 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `Expo` 在摇杆曲线里是指数曲线，不是相机曝光。 |
| `MC_ACRO_EXPO_Y` | Acro 模式偏航曝光系数 | Acro 模式偏航指数曲线系数 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正 Expo 术语。 |
| `MC_ACRO_SUPEXPO` | 特技模式横滚、俯仰超级展因子 | Acro 模式横滚、俯仰超级指数曲线系数 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | `SuperExpo` 是超级指数曲线系数。 |
| `MC_ACRO_SUPEXPOY` | Acro 模式偏航超级曝光系数 | Acro 模式偏航超级指数曲线系数 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 修正 SuperExpo 术语。 |
| `COM_RC_STICK_OV` | 操纵杆输入移动超过此阈值，则自动驾驶仪将接管控制权 | 遥控器摇杆输入超过该阈值，则飞手将通过摇杆超控自动模式并接管控制权 | `src/FirmwarePlugin/PX4/PX4ParameterFactMetaData.xml` | 原译文反了控制权方向。 |
