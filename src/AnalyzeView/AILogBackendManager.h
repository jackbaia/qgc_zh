/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtNetwork/QNetworkAccessManager>

Q_DECLARE_LOGGING_CATEGORY(AILogBackendManagerLog)

class QNetworkReply;

class AILogBackendManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit AILogBackendManager(QObject *parent = nullptr);
    ~AILogBackendManager();

    static AILogBackendManager *instance();

    bool running() const { return _running; }
    bool starting() const { return _starting; }
    QString statusText() const { return _statusText; }
    QString errorText() const { return _errorText; }

    Q_INVOKABLE void ensureStarted();
    Q_INVOKABLE void stopIfOwnedByQGC();
    Q_INVOKABLE void checkHealth();

signals:
    void runningChanged();
    void startingChanged();
    void statusTextChanged();
    void errorTextChanged();
    void backendReady();
    void backendFailed(const QString &error);

private:
    void _checkHealthThenStartIfNeeded();
    void _startBackendProcess();
    void _pollHealthAfterStart();
    void _handleHealthReply(QNetworkReply *reply, bool startIfNeeded);
    void _setRunning(bool running);
    void _setStarting(bool starting);
    void _setStatusText(const QString &text);
    void _setErrorText(const QString &text);
    QString _backendExecutablePath() const;
    QStringList _backendArguments() const;
    QString _backendWorkingDirectory() const;
    bool _healthPayloadIsOurs(const QByteArray &payload) const;
    QString _processOutputSnippet() const;

    QProcess *_process = nullptr;
    QNetworkAccessManager _networkManager;
    bool _running = false;
    bool _starting = false;
    bool _ownedByQGC = false;
    qint64 _processId = 0;
    QString _statusText;
    QString _errorText;
    int _healthPollsRemaining = 0;

    static constexpr int kBackendPort = 8765;
    static constexpr int kHealthPollIntervalMs = 500;
    static constexpr int kStartupTimeoutMs = 8000;
};
