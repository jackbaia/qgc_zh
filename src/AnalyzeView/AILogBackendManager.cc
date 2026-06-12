/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AILogBackendManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

QGC_LOGGING_CATEGORY(AILogBackendManagerLog, "qgc.analyzeview.ailogbackendmanager")

Q_APPLICATION_STATIC(AILogBackendManager, _aiLogBackendManagerInstance);

AILogBackendManager::AILogBackendManager(QObject *parent)
    : QObject(parent)
{
    _setStatusText(QStringLiteral("\u5c1a\u672a\u68c0\u67e5 ULog \u5206\u6790\u670d\u52a1"));

    if (QCoreApplication::instance()) {
        (void) connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &AILogBackendManager::stopIfOwnedByQGC);
    }
}

AILogBackendManager::~AILogBackendManager()
{
    stopIfOwnedByQGC();
}

AILogBackendManager *AILogBackendManager::instance()
{
    return _aiLogBackendManagerInstance();
}

void AILogBackendManager::ensureStarted()
{
    if (_running || _starting) {
        return;
    }

    _setErrorText(QString());
    _setStatusText(QStringLiteral("\u6b63\u5728\u68c0\u67e5 ULog \u5206\u6790\u670d\u52a1..."));
    _checkHealthThenStartIfNeeded();
}

void AILogBackendManager::stopIfOwnedByQGC()
{
    if (!_ownedByQGC || !_process) {
        return;
    }

#if defined(Q_OS_WIN)
    if (_processId > 0) {
        (void) QProcess::execute(QStringLiteral("taskkill"), {
            QStringLiteral("/PID"),
            QString::number(_processId),
            QStringLiteral("/T"),
            QStringLiteral("/F")
        });
    }
#endif

    if (_process->state() != QProcess::NotRunning) {
        _process->terminate();
        if (!_process->waitForFinished(2000)) {
            _process->kill();
            (void) _process->waitForFinished(1000);
        }
    }

    _ownedByQGC = false;
    _processId = 0;
    _setStarting(false);
    _setRunning(false);
}

void AILogBackendManager::checkHealth()
{
    _setStatusText(QStringLiteral("\u6b63\u5728\u68c0\u67e5 ULog \u5206\u6790\u670d\u52a1..."));
    QNetworkReply *reply = _networkManager.get(QNetworkRequest(QUrl(QStringLiteral("http://127.0.0.1:%1/health").arg(kBackendPort))));
    (void) connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        _handleHealthReply(reply, false);
    });
}

void AILogBackendManager::_checkHealthThenStartIfNeeded()
{
    QNetworkReply *reply = _networkManager.get(QNetworkRequest(QUrl(QStringLiteral("http://127.0.0.1:%1/health").arg(kBackendPort))));
    (void) connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        _handleHealthReply(reply, true);
    });
}

void AILogBackendManager::_startBackendProcess()
{
    const QString backendPath = _backendExecutablePath();
    const QByteArray devCommand = qgetenv("QGC_AI_ULOG_BACKEND_DEV_CMD");

    QString program;
    QStringList arguments;
    QString workingDirectory;

    if (!backendPath.isEmpty()) {
        program = backendPath;
        arguments = _backendArguments();
        workingDirectory = _backendWorkingDirectory();
    } else if (!devCommand.isEmpty()) {
        const QStringList parts = QProcess::splitCommand(QString::fromLocal8Bit(devCommand));
        if (!parts.isEmpty()) {
            program = parts.first();
            arguments = parts.mid(1);
            workingDirectory = QString::fromLocal8Bit(qgetenv("QGC_AI_ULOG_BACKEND_DEV_DIR"));
        }
    }

    if (program.isEmpty()) {
        const QString error = QStringLiteral("\u672a\u627e\u5230 ULog \u5206\u6790\u540e\u7aef\u7a0b\u5e8f\uff0c\u8bf7\u786e\u8ba4 ai_ulog_backend \u5df2\u968f QGC \u90e8\u7f72");
        _setErrorText(error);
        _setStatusText(error);
        emit backendFailed(error);
        return;
    }

    if (_process) {
        _process->deleteLater();
    }

    _process = new QProcess(this);
    _process->setProgram(program);
    _process->setArguments(arguments);
    if (!workingDirectory.isEmpty()) {
        _process->setWorkingDirectory(workingDirectory);
    }
    _process->setProcessChannelMode(QProcess::MergedChannels);

    (void) connect(_process, &QProcess::errorOccurred, this, [this, program](QProcess::ProcessError) {
        const QString error = QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u542f\u52a8\u5931\u8d25: %1\n%2\n%3")
            .arg(program, _process ? _process->errorString() : QString(), _processOutputSnippet());
        _setStarting(false);
        _setRunning(false);
        _setErrorText(error);
        _setStatusText(error);
        emit backendFailed(error);
    });

    (void) connect(_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, program](int exitCode, QProcess::ExitStatus) {
        if (_starting || _running) {
            const QString error = QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u5df2\u9000\u51fa: %1, exitCode=%2\n%3")
                .arg(program)
                .arg(exitCode)
                .arg(_processOutputSnippet());
            _setStarting(false);
            _setRunning(false);
            _setErrorText(error);
            _setStatusText(error);
            emit backendFailed(error);
        }
    });

    _setStarting(true);
    _setStatusText(QStringLiteral("\u6b63\u5728\u542f\u52a8 ULog \u5206\u6790\u670d\u52a1..."));
    _process->start();

    if (!_process->waitForStarted(3000)) {
        const QString error = QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u542f\u52a8\u5931\u8d25: %1\n%2")
            .arg(program, _process->errorString());
        _setStarting(false);
        _setErrorText(error);
        _setStatusText(error);
        emit backendFailed(error);
        return;
    }

    _ownedByQGC = true;
    _processId = _process->processId();
    _healthPollsRemaining = kStartupTimeoutMs / kHealthPollIntervalMs;
    _pollHealthAfterStart();
}

void AILogBackendManager::_pollHealthAfterStart()
{
    if (_healthPollsRemaining-- <= 0) {
        const QString error = QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u542f\u52a8\u8d85\u65f6\uff0c8 \u79d2\u5185 /health \u4e0d\u53ef\u8bbf\u95ee\n%1").arg(_processOutputSnippet());
        _setStarting(false);
        _setRunning(false);
        _setErrorText(error);
        _setStatusText(error);
        emit backendFailed(error);
        return;
    }

    QNetworkReply *reply = _networkManager.get(QNetworkRequest(QUrl(QStringLiteral("http://127.0.0.1:%1/health").arg(kBackendPort))));
    (void) connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if ((reply->error() == QNetworkReply::NoError) && _healthPayloadIsOurs(reply->readAll())) {
            reply->deleteLater();
            _setStarting(false);
            _setRunning(true);
            _setErrorText(QString());
            _setStatusText(QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u5df2\u8fd0\u884c"));
            emit backendReady();
            return;
        }

        reply->deleteLater();
        QTimer::singleShot(kHealthPollIntervalMs, this, &AILogBackendManager::_pollHealthAfterStart);
    });
}

void AILogBackendManager::_handleHealthReply(QNetworkReply *reply, bool startIfNeeded)
{
    const QNetworkReply::NetworkError replyError = reply->error();
    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    if (replyError == QNetworkReply::NoError) {
        if (_healthPayloadIsOurs(payload)) {
            _ownedByQGC = false;
            _setStarting(false);
            _setRunning(true);
            _setErrorText(QString());
            _setStatusText(QStringLiteral("\u68c0\u6d4b\u5230 ULog \u5206\u6790\u670d\u52a1\u5df2\u8fd0\u884c"));
            emit backendReady();
        } else {
            const QString error = QStringLiteral("\u7aef\u53e3 8765 \u5df2\u88ab\u5360\u7528\uff0c\u4f46\u4e0d\u662f ULog \u5206\u6790\u670d\u52a1");
            _setRunning(false);
            _setStarting(false);
            _setErrorText(error);
            _setStatusText(error);
            emit backendFailed(error);
        }
        return;
    }

    _setRunning(false);
    if (startIfNeeded) {
        _startBackendProcess();
    } else {
        const QString error = QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u672a\u8fd0\u884c");
        _setStatusText(error);
        _setErrorText(error);
        emit backendFailed(error);
    }
}

void AILogBackendManager::_setRunning(bool running)
{
    if (_running == running) {
        return;
    }
    _running = running;
    emit runningChanged();
}

void AILogBackendManager::_setStarting(bool starting)
{
    if (_starting == starting) {
        return;
    }
    _starting = starting;
    emit startingChanged();
}

void AILogBackendManager::_setStatusText(const QString &text)
{
    if (_statusText == text) {
        return;
    }
    _statusText = text;
    emit statusTextChanged();
}

void AILogBackendManager::_setErrorText(const QString &text)
{
    if (_errorText == text) {
        return;
    }
    _errorText = text;
    emit errorTextChanged();
}

QString AILogBackendManager::_backendExecutablePath() const
{
    const QDir appDir(QCoreApplication::applicationDirPath());
#if defined(Q_OS_WIN)
    const QString executableName = QStringLiteral("ai_ulog_backend.exe");
#else
    const QString executableName = QStringLiteral("ai_ulog_backend");
#endif

    const QString path = appDir.filePath(QStringLiteral("ai_ulog_backend/%1").arg(executableName));
    return QFileInfo(path).isExecutable() ? path : QString();
}

QStringList AILogBackendManager::_backendArguments() const
{
    return QStringList() << QStringLiteral("--host") << QStringLiteral("127.0.0.1") << QStringLiteral("--port") << QString::number(kBackendPort);
}

QString AILogBackendManager::_backendWorkingDirectory() const
{
    const QString backendPath = _backendExecutablePath();
    return backendPath.isEmpty() ? QString() : QFileInfo(backendPath).absolutePath();
}

bool AILogBackendManager::_healthPayloadIsOurs(const QByteArray &payload) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if ((parseError.error != QJsonParseError::NoError) || !document.isObject()) {
        return false;
    }

    const QJsonObject object = document.object();
    return (object.value(QStringLiteral("status")).toString() == QStringLiteral("ok")) &&
           (object.value(QStringLiteral("service")).toString() == QStringLiteral("qgc_ulog_backend"));
}

QString AILogBackendManager::_processOutputSnippet() const
{
    if (!_process) {
        return QString();
    }

    const QString output = QString::fromUtf8(_process->readAll());
    return output.left(2000);
}
