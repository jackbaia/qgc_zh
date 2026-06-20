/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QBitArray>
#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>

#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(AILogAnalyzeControllerLog)

class QmlObjectListModel;
class QTimer;
class Vehicle;

class AILogEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint logId READ logId CONSTANT)
    Q_PROPERTY(QDateTime timeUtc READ timeUtc NOTIFY timeUtcChanged)
    Q_PROPERTY(uint sizeBytes READ sizeBytes NOTIFY sizeBytesChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit AILogEntry(uint logId, QObject *parent = nullptr);

    uint logId() const { return _logId; }
    QDateTime timeUtc() const { return _timeUtc; }
    uint sizeBytes() const { return _sizeBytes; }
    QString displayName() const;
    QString status() const { return _status; }
    bool received() const { return _received; }

    void setTimeUtc(const QDateTime &timeUtc);
    void setSizeBytes(uint sizeBytes);
    void setStatus(const QString &status);
    void setReceived(bool received) { _received = received; }

signals:
    void timeUtcChanged();
    void sizeBytesChanged();
    void displayNameChanged();
    void statusChanged();

private:
    uint _logId = 0;
    QDateTime _timeUtc;
    uint _sizeBytes = 0;
    QString _status;
    bool _received = false;
};

class AILogAnalyzeController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QmlObjectListModel *logEntries READ logEntries CONSTANT)
    Q_PROPERTY(int selectedLogIndex READ selectedLogIndex WRITE setSelectedLogIndex NOTIFY selectedLogIndexChanged)
    Q_PROPERTY(QString resultText READ resultText NOTIFY resultTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString backendStatusText READ backendStatusText NOTIFY backendStatusTextChanged)
    Q_PROPERTY(QString backendErrorText READ backendErrorText NOTIFY backendErrorTextChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool backendRunning READ backendRunning NOTIFY backendRunningChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY hasResultChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    explicit AILogAnalyzeController(QObject *parent = nullptr);
    ~AILogAnalyzeController();

    QmlObjectListModel *logEntries() const { return _logEntries; }
    int selectedLogIndex() const { return _selectedLogIndex; }
    QString resultText() const { return _resultText; }
    QString errorText() const { return _errorText; }
    QString statusText() const { return _statusText; }
    QString backendStatusText() const;
    QString backendErrorText() const;
    bool busy() const { return _busy; }
    bool backendRunning() const;
    bool hasResult() const { return _hasResult; }
    double progress() const { return _progress; }

    void setSelectedLogIndex(int selectedLogIndex);

    Q_INVOKABLE void refreshLogList();
    Q_INVOKABLE void analyzeSelectedLog();
    Q_INVOKABLE void saveResultJson(const QString &filePath);

signals:
    void selectedLogIndexChanged();
    void resultTextChanged();
    void errorTextChanged();
    void statusTextChanged();
    void backendStatusTextChanged();
    void backendErrorTextChanged();
    void busyChanged();
    void backendRunningChanged();
    void hasResultChanged();
    void progressChanged();

private slots:
    void _setActiveVehicle(Vehicle *vehicle);
    void _logEntry(uint32_t timeUtc, uint32_t size, uint16_t id, uint16_t numLogs, uint16_t lastLogNum);
    void _logData(uint32_t offset, uint16_t id, uint8_t count, const uint8_t *data);
    void _processTimeout();

private:
    struct MemoryDownloadData {
        explicit MemoryDownloadData(AILogEntry *logEntry);

        void advanceChunk();
        uint32_t chunkBins() const;
        uint32_t numChunks() const;
        bool chunkEquals(bool value) const;

        uint id = 0;
        AILogEntry *entry = nullptr;
        QByteArray bytes;
        QBitArray chunkTable;
        uint32_t currentChunk = 0;
        uint32_t written = 0;
        uint32_t rateBytes = 0;
        qreal rateAvg = 0.;
        QElapsedTimer elapsed;

        static constexpr uint32_t kTableBins = 512;
        static constexpr uint32_t kChunkSize = kTableBins * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    };

    enum class Operation {
        Idle,
        Listing,
        Downloading,
        Uploading
    };

    void _setResultText(const QString &resultText);
    void _setErrorText(const QString &errorText);
    void _setStatusText(const QString &statusText);
    void _setBusy(bool busy);
    void _setHasResult(bool hasResult);
    void _setProgress(double progress);
    void _setOperation(Operation operation);
    void _finishList(bool success, const QString &message = QString());
    void _finishDownloadWithError(const QString &message);
    void _uploadLogBytes(uint logId, const QByteArray &logBytes);
    void _requestLogList(uint32_t start, uint32_t end);
    void _requestLogData(uint16_t id, uint32_t offset, uint32_t count);
    void _requestLogEnd();
    void _findMissingEntries();
    void _findMissingData();
    bool _entriesComplete() const;
    bool _chunkComplete() const;
    bool _logComplete() const;
    void _startDownload(AILogEntry *entry);
    void _startSelectedLogAnalysisAfterBackendReady();
    AILogEntry *_selectedEntry() const;
    static QString _localFilePathFromUrl(const QString &filePath);
    void _clearLogEntries();

    QNetworkAccessManager _networkAccessManager;
    QmlObjectListModel *_logEntries = nullptr;
    QTimer *_timer = nullptr;
    QPointer<Vehicle> _vehicle;
    QMetaObject::Connection _logEntryConnection;
    QMetaObject::Connection _logDataConnection;
    std::unique_ptr<MemoryDownloadData> _downloadData;

    QString _resultText;
    QString _errorText;
    QString _statusText;
    int _selectedLogIndex = -1;
    int _apmOffset = 0;
    int _retries = 0;
    bool _busy = false;
    bool _startAnalysisWhenBackendReady = false;
    bool _hasResult = false;
    double _progress = 0.0;
    Operation _operation = Operation::Idle;

    static constexpr uint32_t kTimeoutMs = 500;
    static constexpr uint32_t kGuiRateMs = 17;
    static constexpr uint32_t kRequestLogListTimeoutMs = 5000;
};
