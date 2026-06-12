# PX4 ULog Analyzer Service

这是给 QGroundControl “AI 日志分析”页面调用的本地 PX4 `.ulg` 日志解析后端。当前版本只做 ULog 解析并返回结构化 JSON，不调用 GPT，也不修改飞控参数。

## 接口

- `GET /health`：健康检查，返回 `{"status":"ok","service":"qgc_ulog_backend","version":"0.1.0"}`。
- `POST /analyze_ulog`：上传 `.ulg` 文件，字段名为 `file`，返回完整 JSON 分析结果。

## 开发启动

```powershell
cd work\ulog_backend
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
uvicorn main:app --host 127.0.0.1 --port 8765
```

## 打包为 QGC 绿色版随附后端

```powershell
cd work\ulog_backend
.\.venv\Scripts\python.exe -m pip install pyinstaller
.\.venv\Scripts\python.exe -m PyInstaller -F --console main.py -n ai_ulog_backend
```

把生成的 `dist\ai_ulog_backend.exe` 放到 QGC 程序同级目录：

```text
QGroundControl.exe
ai_ulog_backend\ai_ulog_backend.exe
```

QGC 启动后会先访问 `http://127.0.0.1:8765/health`。如果检测到已经运行的是 `qgc_ulog_backend`，就直接复用；如果没有运行，则用 `QProcess` 启动随附的 `ai_ulog_backend.exe`。QGC 退出时只会关闭自己启动的后端进程。

## 开发模式环境变量

如果没有随附 exe，也可以设置：

```powershell
$env:QGC_AI_ULOG_BACKEND_DEV_CMD="python -m uvicorn main:app --host 127.0.0.1 --port 8765"
$env:QGC_AI_ULOG_BACKEND_DEV_DIR="C:\path\to\work\ulog_backend"
```
