# QGC 东北话语音包

## 功能说明

“东北话语音包优先”模式用于在 QGroundControl 发生常见飞控播报事件时，优先播放用户录制的本地 `.wav` 文件。QGC 不生成东北话 TTS，不调用 GPT，也不修改任何飞控参数。

当某条播报文本能识别出事件 key，并且语音包目录中存在对应音频文件时，QGC 播放该音频；如果找不到文件或没有识别到事件，则回退到原有 TTS。

## 目录结构

默认目录位于 QGC 可执行程序旁边：

```text
QGroundControl/
├── QGroundControl.exe
└── audio/
    └── dongbei/
        ├── manifest.json
        ├── arm_success.wav
        ├── disarm.wav
        ├── takeoff.wav
        ├── landing.wav
        ├── mode_manual.wav
        ├── mode_position.wav
        ├── mode_altitude.wav
        ├── mode_offboard.wav
        ├── mode_rtl.wav
        ├── battery_low.wav
        └── ...
```

也可以在 QGC 设置里指定其他目录。

## manifest.json 示例

`manifest.json` 是可选文件。没有 manifest 时，QGC 会按事件 key 查找同名 `.wav`，例如 `battery_low` 对应 `battery_low.wav`。

```json
{
  "name": "dongbei_audio_pack",
  "displayName": "东北话语音包",
  "version": "1.0.0",
  "events": {
    "arm_success": {
      "file": "arm_success.wav",
      "fallbackText": "解锁成功"
    },
    "disarm": {
      "file": "disarm.wav",
      "fallbackText": "已经上锁"
    },
    "battery_low": {
      "file": "battery_low.wav",
      "fallbackText": "电池电量低"
    },
    "failsafe": {
      "file": "failsafe.wav",
      "fallbackText": "失效保护已触发"
    }
  }
}
```

## 支持的事件 Key

```text
arm_success
disarm
takeoff
landing
mode_manual
mode_position
mode_altitude
mode_stabilized
mode_offboard
mode_rtl
mode_mission
battery_low
battery_critical
gps_lost
gps_recovered
rc_lost
rc_recovered
failsafe
prearm_failed
ekf_warning
compass_error
airspeed_warning
emergency_kill
log_analysis_started
log_analysis_finished
```

## 推荐录音文案

危险告警应清晰、短、直接，不建议过度娱乐化。

| 文件名 | 推荐文案 |
|---|---|
| `arm_success.wav` | 解锁成了，注意安全。 |
| `disarm.wav` | 已经上锁了。 |
| `takeoff.wav` | 开始起飞，注意观察。 |
| `landing.wav` | 准备落地，注意高度。 |
| `mode_manual.wav` | 切到手动模式了。 |
| `mode_position.wav` | 切到位置模式了。 |
| `mode_altitude.wav` | 切到高度模式了。 |
| `mode_stabilized.wav` | 切到自稳模式了。 |
| `mode_offboard.wav` | 切到外部控制模式了。 |
| `mode_rtl.wav` | 开始返航了。 |
| `mode_mission.wav` | 开始执行航线任务了。 |
| `battery_low.wav` | 电池电量低了，抓紧返航。 |
| `battery_critical.wav` | 电池严重不足，马上降落。 |
| `gps_lost.wav` | GPS 信号丢了，注意定位。 |
| `gps_recovered.wav` | GPS 信号恢复了。 |
| `rc_lost.wav` | 遥控信号丢了，注意接管。 |
| `rc_recovered.wav` | 遥控信号恢复了。 |
| `failsafe.wav` | 失效保护触发了，注意飞行器状态。 |
| `prearm_failed.wav` | 解锁检查没通过，先别起飞。 |
| `ekf_warning.wav` | 状态估计不太对，检查定位。 |
| `compass_error.wav` | 磁罗盘有问题，检查校准。 |
| `airspeed_warning.wav` | 空速数据异常，注意固定翼飞行。 |
| `emergency_kill.wav` | 紧急停机已触发，注意安全。 |
| `log_analysis_started.wav` | 开始分析飞行日志。 |
| `log_analysis_finished.wav` | 日志分析完成。 |

## 音频格式

第一版优先支持 `.wav`：

```text
WAV
16-bit PCM
44100 Hz 或 48000 Hz
单声道或双声道均可
文件名使用英文小写和下划线
```

## 如何启用

打开 QGC：

```text
Application Settings → General → Voice Style
```

选择：

```text
Dongbei audio pack first
```

中文界面显示为：

```text
东北话语音包优先
```

如果语音包目录不是默认位置，在 `Dongbei Audio Pack Path` 中选择对应文件夹，然后点击 `Test Audio Pack`。测试会优先播放 `arm_success.wav`；如果文件不存在，会回退到 TTS。

## 缺失音频的回退逻辑

如果事件 key 对应的文件不存在，QGC 不报错、不阻塞主线程，直接回退到原有 TTS 播报。

例如低电量播报识别为：

```text
battery_low
```

QGC 会依次尝试：

```text
manifest.json 中 battery_low.file 指定的文件
battery_low.wav
原有 TTS
```

## 新增事件音频

1. 在语音包目录中放入新的 `.wav` 文件。
2. 文件名建议与事件 key 一致，例如 `gps_recovered.wav`。
3. 如需自定义文件名，在 `manifest.json` 的 `events` 中添加映射。
4. 重启 QGC 或重新触发事件即可验证。
