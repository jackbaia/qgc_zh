/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AILogAnalyzeController.h"
#include "AILogBackendManager.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QtMath>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <cstring>
#include <limits>

QGC_LOGGING_CATEGORY(AILogAnalyzeControllerLog, "qgc.analyzeview.ailoganalyzecontroller")

AILogEntry::AILogEntry(uint logId, QObject *parent)
    : QObject(parent)
    , _logId(logId)
    , _status(QStringLiteral("\u7b49\u5f85"))
{
}

void AILogEntry::setTimeUtc(const QDateTime &timeUtc)
{
    if (_timeUtc == timeUtc) {
        return;
    }

    _timeUtc = timeUtc;
    emit timeUtcChanged();
    emit displayNameChanged();
}

void AILogEntry::setSizeBytes(uint sizeBytes)
{
    if (_sizeBytes == sizeBytes) {
        return;
    }

    _sizeBytes = sizeBytes;
    emit sizeBytesChanged();
    emit displayNameChanged();
}

void AILogEntry::setStatus(const QString &status)
{
    if (_status == status) {
        return;
    }

    _status = status;
    emit statusChanged();
}

QString AILogEntry::displayName() const
{
    const QString timeText = _timeUtc.isValid() && (_timeUtc.date().year() >= 2010)
        ? _timeUtc.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))
        : QStringLiteral("UnknownDate");
    return QStringLiteral("log_%1_%2 (%3)").arg(_logId).arg(timeText).arg(qgcApp()->bigSizeToString(_sizeBytes));
}

AILogAnalyzeController::MemoryDownloadData::MemoryDownloadData(AILogEntry *logEntry)
    : id(logEntry ? logEntry->logId() : 0)
    , entry(logEntry)
{
}

void AILogAnalyzeController::MemoryDownloadData::advanceChunk()
{
    ++currentChunk;
    chunkTable = QBitArray(chunkBins(), false);
}

uint32_t AILogAnalyzeController::MemoryDownloadData::chunkBins() const
{
    if (!entry || (entry->sizeBytes() <= (currentChunk * kChunkSize))) {
        return 0;
    }

    const qreal num = static_cast<qreal>(entry->sizeBytes() - (currentChunk * kChunkSize)) / static_cast<qreal>(MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    return qMin(static_cast<uint32_t>(qCeil(num)), kTableBins);
}

uint32_t AILogAnalyzeController::MemoryDownloadData::numChunks() const
{
    if (!entry || entry->sizeBytes() == 0) {
        return 0;
    }

    const qreal num = static_cast<qreal>(entry->sizeBytes()) / static_cast<qreal>(kChunkSize);
    return qCeil(num);
}

bool AILogAnalyzeController::MemoryDownloadData::chunkEquals(bool value) const
{
    return chunkTable == QBitArray(chunkTable.size(), value);
}

AILogAnalyzeController::AILogAnalyzeController(QObject *parent)
    : QObject(parent)
    , _logEntries(new QmlObjectListModel(this))
    , _timer(new QTimer(this))
{
    _timer->setSingleShot(false);

    (void) connect(_timer, &QTimer::timeout, this, &AILogAnalyzeController::_processTimeout);
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &AILogAnalyzeController::_setActiveVehicle);
    (void) connect(AILogBackendManager::instance(), &AILogBackendManager::statusTextChanged, this, &AILogAnalyzeController::backendStatusTextChanged);
    (void) connect(AILogBackendManager::instance(), &AILogBackendManager::errorTextChanged, this, &AILogAnalyzeController::backendErrorTextChanged);
    (void) connect(AILogBackendManager::instance(), &AILogBackendManager::runningChanged, this, &AILogAnalyzeController::backendRunningChanged);
    (void) connect(AILogBackendManager::instance(), &AILogBackendManager::startingChanged, this, &AILogAnalyzeController::backendRunningChanged);
    (void) connect(AILogBackendManager::instance(), &AILogBackendManager::backendReady, this, [this]() {
        emit backendStatusTextChanged();
        emit backendRunningChanged();
        if (_startAnalysisWhenBackendReady) {
            _startAnalysisWhenBackendReady = false;
            _setOperation(Operation::Idle);
            _startSelectedLogAnalysisAfterBackendReady();
        }
    });
    (void) connect(AILogBackendManager::instance(), &AILogBackendManager::backendFailed, this, [this](const QString &error) {
        emit backendStatusTextChanged();
        emit backendErrorTextChanged();
        emit backendRunningChanged();
        if (_startAnalysisWhenBackendReady) {
            _startAnalysisWhenBackendReady = false;
            _setOperation(Operation::Idle);
            const QString message = error.isEmpty()
                ? QStringLiteral("ULog \u5206\u6790\u670d\u52a1\u542f\u52a8\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5 ai_ulog_backend \u662f\u5426\u5df2\u6b63\u786e\u90e8\u7f72")
                : error;
            _setStatusText(message);
            _setErrorText(message);
        }
    });

    AILogBackendManager::instance()->checkHealth();

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
    _setStatusText(_vehicle ? QStringLiteral("\u5df2\u8fde\u63a5\u98de\u63a7") : QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7"));
}

AILogAnalyzeController::~AILogAnalyzeController()
{
    _timer->stop();
    if (_operation == Operation::Downloading) {
        _requestLogEnd();
    }
}

void AILogAnalyzeController::setSelectedLogIndex(int selectedLogIndex)
{
    if (_selectedLogIndex == selectedLogIndex) {
        return;
    }

    _selectedLogIndex = selectedLogIndex;
    emit selectedLogIndexChanged();
}

QString AILogAnalyzeController::backendStatusText() const
{
    return AILogBackendManager::instance()->statusText();
}

QString AILogAnalyzeController::backendErrorText() const
{
    return AILogBackendManager::instance()->errorText();
}

bool AILogAnalyzeController::backendRunning() const
{
    return AILogBackendManager::instance()->running();
}

void AILogAnalyzeController::refreshLogList()
{
    if (_busy) {
        return;
    }

    if (!_vehicle) {
        _clearLogEntries();
        _setStatusText(QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7\uff0c\u65e0\u6cd5\u8bfb\u53d6\u65e5\u5fd7"));
        _setErrorText(QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7\uff0c\u65e0\u6cd5\u8bfb\u53d6\u65e5\u5fd7"));
        return;
    }

    _clearLogEntries();
    setSelectedLogIndex(-1);
    _setResultText(QString());
    _setErrorText(QString());
    _setHasResult(false);
    _setProgress(0.0);
    _apmOffset = 0;
    _retries = 0;

    _setOperation(Operation::Listing);
    _setStatusText(QStringLiteral("\u6b63\u5728\u8bfb\u53d6\u65e5\u5fd7\u5217\u8868..."));
    _requestLogList(0, 0xffff);
}

void AILogAnalyzeController::analyzeSelectedLog()
{
    if (_busy) {
        return;
    }

    if (!_vehicle) {
        _setErrorText(QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7\uff0c\u65e0\u6cd5\u8bfb\u53d6\u65e5\u5fd7"));
        return;
    }

    AILogEntry *entry = _selectedEntry();
    if (!entry) {
        _setErrorText(QStringLiteral("\u8bf7\u5148\u9009\u62e9\u4e00\u6761\u98de\u63a7\u65e5\u5fd7"));
        return;
    }

    if (!AILogBackendManager::instance()->running()) {
        _startAnalysisWhenBackendReady = true;
        _setResultText(QString());
        _setErrorText(QString());
        _setHasResult(false);
        _setProgress(0.0);
        _setOperation(Operation::Uploading);
        _setStatusText(QStringLiteral("\u6b63\u5728\u542f\u52a8 ULog \u5206\u6790\u670d\u52a1..."));
        AILogBackendManager::instance()->ensureStarted();
        return;
    }

    _setResultText(QString());
    _setErrorText(QString());
    _setHasResult(false);
    _startDownload(entry);
}

void AILogAnalyzeController::saveResultJson(const QString &filePath)
{
    _setErrorText(QString());

    if (_resultText.isEmpty()) {
        _setErrorText(QStringLiteral("\u5f53\u524d\u6ca1\u6709\u53ef\u4fdd\u5b58\u7684\u5206\u6790\u7ed3\u679c"));
        return;
    }

    QString localFilePath = _localFilePathFromUrl(filePath);
    if (localFilePath.isEmpty()) {
        _setErrorText(QStringLiteral("\u8bf7\u9009\u62e9 JSON \u4fdd\u5b58\u8def\u5f84"));
        return;
    }

    if (!localFilePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        localFilePath += QStringLiteral(".json");
    }

    QFile file(localFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        _setErrorText(QStringLiteral("\u4fdd\u5b58 JSON \u6587\u4ef6\u5931\u8d25: %1").arg(file.errorString()));
        return;
    }

    const QByteArray data = _resultText.toUtf8();
    if (file.write(data) != data.size()) {
        _setErrorText(QStringLiteral("\u4fdd\u5b58 JSON \u6587\u4ef6\u5931\u8d25: %1").arg(file.errorString()));
        return;
    }

    _setStatusText(QStringLiteral("JSON \u6587\u4ef6\u5df2\u4fdd\u5b58"));
}

void AILogAnalyzeController::_setActiveVehicle(Vehicle *vehicle)
{
    if (_vehicle == vehicle) {
        return;
    }

    // Vehicle replacement can happen while the old QObject is being torn down
    // (for example after a reboot command). Disconnect by connection handle and
    // do not send another MAVLink command through the closing Vehicle.
    (void) QObject::disconnect(_logEntryConnection);
    (void) QObject::disconnect(_logDataConnection);
    _logEntryConnection = {};
    _logDataConnection = {};

    _timer->stop();
    _downloadData.reset();
    _vehicle = vehicle;
    _setOperation(Operation::Idle);
    _setProgress(0.0);
    _clearLogEntries();
    setSelectedLogIndex(-1);

    if (_vehicle) {
        _logEntryConnection = connect(_vehicle, &Vehicle::logEntry, this, &AILogAnalyzeController::_logEntry);
        _logDataConnection = connect(_vehicle, &Vehicle::logData, this, &AILogAnalyzeController::_logData);
        _setStatusText(QStringLiteral("\u5df2\u8fde\u63a5\u98de\u63a7"));
    } else {
        _setStatusText(QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7"));
    }
}

void AILogAnalyzeController::_logEntry(uint32_t timeUtc, uint32_t size, uint16_t id, uint16_t numLogs, uint16_t lastLogNum)
{
    Q_UNUSED(lastLogNum)

    if (_operation != Operation::Listing) {
        return;
    }

    if ((_logEntries->count() == 0) && (numLogs > 0)) {
        if (_vehicle && (_vehicle->firmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA)) {
            _apmOffset = 1;
        }

        for (int i = 0; i < numLogs; ++i) {
            AILogEntry *entry = new AILogEntry(i, this);
            _logEntries->append(entry);
        }
    }

    if (numLogs > 0) {
        if ((size > 0) || (!_vehicle || (_vehicle->firmwareType() != MAV_AUTOPILOT_ARDUPILOTMEGA))) {
            id -= _apmOffset;
            if (id < _logEntries->count()) {
                AILogEntry *entry = _logEntries->value<AILogEntry*>(id);
                if (entry) {
                    entry->setSizeBytes(size);
                    entry->setTimeUtc(QDateTime::fromSecsSinceEpoch(timeUtc));
                    entry->setReceived(true);
                    entry->setStatus(QStringLiteral("\u53ef\u7528"));
                }
            }
        }
    } else {
        _finishList(true, QStringLiteral("\u5f53\u524d\u98de\u63a7\u6ca1\u6709\u53ef\u7528\u65e5\u5fd7"));
        return;
    }

    _retries = 0;
    if (_entriesComplete()) {
        _finishList(true);
    } else {
        _timer->start(kTimeoutMs);
    }
}

void AILogAnalyzeController::_logData(uint32_t offset, uint16_t id, uint8_t count, const uint8_t *data)
{
    if ((_operation != Operation::Downloading) || !_downloadData) {
        return;
    }

    if ((id < _apmOffset) || !data) {
        qCWarning(AILogAnalyzeControllerLog) << "Ignored invalid log data packet" << id << count << data;
        return;
    }
    id -= _apmOffset;
    if (_downloadData->id != id) {
        qCWarning(AILogAnalyzeControllerLog) << "Received log data for wrong log" << id << "expected" << _downloadData->id;
        return;
    }

    if ((offset % MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) != 0) {
        qCWarning(AILogAnalyzeControllerLog) << "Ignored misaligned incoming packet" << offset;
        return;
    }

    if (!_downloadData->entry || (offset >= _downloadData->entry->sizeBytes())) {
        _finishDownloadWithError(QStringLiteral("\u63a5\u6536\u5230\u8d85\u51fa\u9884\u671f\u8303\u56f4\u7684\u65e5\u5fd7\u6570\u636e"));
        return;
    }

    const uint32_t chunk = offset / MemoryDownloadData::kChunkSize;
    if (chunk != _downloadData->currentChunk) {
        qCWarning(AILogAnalyzeControllerLog) << "Ignored packet for out-of-order chunk" << chunk << _downloadData->currentChunk;
        return;
    }

    const uint16_t bin = (offset - (chunk * MemoryDownloadData::kChunkSize)) / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    if (bin >= _downloadData->chunkTable.size()) {
        _finishDownloadWithError(QStringLiteral("\u65e5\u5fd7\u6570\u636e\u5206\u7247\u8d85\u51fa\u8303\u56f4"));
        return;
    }

    const int packetBytes = qMin<int>(count, MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const int bytesToCopy = qMin(packetBytes, _downloadData->bytes.size() - static_cast<int>(offset));
    if (bytesToCopy <= 0) {
        return;
    }

    const bool alreadyReceived = _downloadData->chunkTable.testBit(bin);
    memcpy(_downloadData->bytes.data() + offset, data, static_cast<size_t>(bytesToCopy));
    _downloadData->chunkTable.setBit(bin);
    if (!alreadyReceived) {
        _downloadData->written = qMin<uint32_t>(_downloadData->written + bytesToCopy, _downloadData->entry->sizeBytes());
        _downloadData->rateBytes += bytesToCopy;
    }

    _retries = 0;
    const double progress = _downloadData->entry->sizeBytes() > 0 ? static_cast<double>(_downloadData->written) / static_cast<double>(_downloadData->entry->sizeBytes()) : 0.0;
    _setProgress(qBound(0.0, progress, 1.0));

    if (_downloadData->elapsed.elapsed() >= kGuiRateMs) {
        const qreal rate = _downloadData->rateBytes / (_downloadData->elapsed.elapsed() / 1000.0);
        _downloadData->rateAvg = (_downloadData->rateAvg * 0.95) + (rate * 0.05);
        _downloadData->rateBytes = 0;
        _setStatusText(QStringLiteral("\u6b63\u5728\u4e0b\u8f7d\u65e5\u5fd7... %1% (%2/s)")
            .arg(qRound(_progress * 100.0))
            .arg(qgcApp()->bigSizeToString(_downloadData->rateAvg)));
        _downloadData->entry->setStatus(QStringLiteral("%1%").arg(qRound(_progress * 100.0)));
        _downloadData->elapsed.start();
    }

    _timer->start(kTimeoutMs);

    if (_logComplete()) {
        QByteArray logBytes = _downloadData->bytes;
        const uint logId = _downloadData->id;
        _downloadData->entry->setStatus(QStringLiteral("\u5df2\u4e0b\u8f7d"));
        _downloadData.reset();
        _timer->stop();
        _requestLogEnd();

        if (logBytes.isEmpty()) {
            _finishDownloadWithError(QStringLiteral("\u65e5\u5fd7\u4e0b\u8f7d\u5b8c\u6210\uff0c\u4f46\u6570\u636e\u4e3a\u7a7a"));
            return;
        }

        _uploadLogBytes(logId, logBytes);
    } else if (_chunkComplete()) {
        _downloadData->advanceChunk();
        _requestLogData(_downloadData->id, _downloadData->currentChunk * MemoryDownloadData::kChunkSize, _downloadData->chunkTable.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    } else if ((bin < (_downloadData->chunkTable.size() - 1)) && _downloadData->chunkTable.at(bin + 1)) {
        _findMissingData();
    }
}

void AILogAnalyzeController::_processTimeout()
{
    if (_operation == Operation::Listing) {
        _findMissingEntries();
    } else if (_operation == Operation::Downloading) {
        _findMissingData();
    }
}

void AILogAnalyzeController::_setResultText(const QString &resultText)
{
    if (_resultText == resultText) {
        return;
    }

    _resultText = resultText;
    emit resultTextChanged();
}

void AILogAnalyzeController::_setErrorText(const QString &errorText)
{
    if (_errorText == errorText) {
        return;
    }

    _errorText = errorText;
    emit errorTextChanged();
}

void AILogAnalyzeController::_setStatusText(const QString &statusText)
{
    if (_statusText == statusText) {
        return;
    }

    _statusText = statusText;
    emit statusTextChanged();
}

void AILogAnalyzeController::_setBusy(bool busy)
{
    if (_busy == busy) {
        return;
    }

    _busy = busy;
    emit busyChanged();
}

void AILogAnalyzeController::_setHasResult(bool hasResult)
{
    if (_hasResult == hasResult) {
        return;
    }

    _hasResult = hasResult;
    emit hasResultChanged();
}

void AILogAnalyzeController::_setProgress(double progress)
{
    progress = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(_progress, progress)) {
        return;
    }

    _progress = progress;
    emit progressChanged();
}

void AILogAnalyzeController::_setOperation(Operation operation)
{
    if (_operation == operation) {
        return;
    }

    _operation = operation;
    _setBusy(operation != Operation::Idle);
    if ((operation == Operation::Listing) || (operation == Operation::Downloading)) {
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
        }
    } else {
        if (_vehicle) {
            _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);
        }
    }
}

void AILogAnalyzeController::_finishList(bool success, const QString &message)
{
    _timer->stop();
    _setOperation(Operation::Idle);
    if (success) {
        const QString status = !message.isEmpty()
            ? message
            : QStringLiteral("\u65e5\u5fd7\u5217\u8868\u8bfb\u53d6\u5b8c\u6210\uff0c\u5171 %1 \u6761").arg(_logEntries->count());
        _setStatusText(status);
        if (_logEntries->count() == 0) {
            _setErrorText(QStringLiteral("\u5f53\u524d\u98de\u63a7\u4e0d\u652f\u6301\u65e5\u5fd7\u8bfb\u53d6\u6216\u672a\u5f00\u542f\u65e5\u5fd7"));
        }
    } else {
        const QString error = !message.isEmpty()
            ? message
            : QStringLiteral("\u5f53\u524d\u98de\u63a7\u4e0d\u652f\u6301\u65e5\u5fd7\u8bfb\u53d6\u6216\u672a\u5f00\u542f\u65e5\u5fd7");
        _setStatusText(error);
        _setErrorText(error);
    }
}

void AILogAnalyzeController::_finishDownloadWithError(const QString &message)
{
    _timer->stop();
    _requestLogEnd();
    if (_downloadData && _downloadData->entry) {
        _downloadData->entry->setStatus(QStringLiteral("\u9519\u8bef"));
    }
    _downloadData.reset();
    _setOperation(Operation::Idle);
    _setStatusText(message);
    _setErrorText(message);
}

void AILogAnalyzeController::_uploadLogBytes(uint logId, const QByteArray &logBytes)
{
    if (!AILogBackendManager::instance()->running()) {
        _setOperation(Operation::Idle);
        _setStatusText(QStringLiteral("\u540e\u7aef\u8fde\u63a5\u5931\u8d25\uff0c\u8bf7\u786e\u8ba4\u672c\u5730 ULog \u5206\u6790\u670d\u52a1\u5df2\u542f\u52a8"));
        _setErrorText(_statusText);
        AILogBackendManager::instance()->ensureStarted();
        return;
    }

    _setOperation(Operation::Uploading);
    _setProgress(1.0);
    _setStatusText(QStringLiteral("\u6b63\u5728\u4e0a\u4f20\u540e\u7aef\u5206\u6790..."));

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"log_%1.ulg\"").arg(logId))
    );
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/octet-stream")));
    filePart.setBody(logBytes);
    multiPart->append(filePart);

    QHttpPart profilePart;
    profilePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QStringLiteral("form-data; name=\"profile\"")));
    profilePart.setBody(QByteArrayLiteral("tunnel"));
    multiPart->append(profilePart);

    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:8765/analyze_ulog")));
    QNetworkReply *reply = _networkAccessManager.post(request, multiPart);
    multiPart->setParent(reply);

    (void) connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray responseBytes = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError) {
            if ((reply->error() == QNetworkReply::ConnectionRefusedError) || (reply->error() == QNetworkReply::HostNotFoundError)) {
                _setErrorText(QStringLiteral("\u540e\u7aef\u8fde\u63a5\u5931\u8d25\uff0c\u8bf7\u786e\u8ba4\u672c\u5730 ULog \u5206\u6790\u670d\u52a1\u5df2\u542f\u52a8"));
            } else {
                _setErrorText(QStringLiteral("\u540e\u7aef\u8bf7\u6c42\u5931\u8d25: %1").arg(reply->errorString()));
            }
            _setStatusText(_errorText);
        } else if ((statusCode < 200) || (statusCode >= 300)) {
            _setErrorText(QStringLiteral("\u540e\u7aef\u8fd4\u56de HTTP %1: \n%2").arg(statusCode).arg(QString::fromUtf8(responseBytes)));
            _setStatusText(_errorText);
        } else {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(responseBytes, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                _setResultText(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));
                _setHasResult(true);
                _setStatusText(QStringLiteral("\u5206\u6790\u5b8c\u6210"));
            } else {
                _setResultText(QString::fromUtf8(responseBytes));
                _setHasResult(!responseBytes.isEmpty());
                _setErrorText(QStringLiteral("\u540e\u7aef\u8fd4\u56de\u7ed3\u679c\u4e0d\u662f\u6807\u51c6 JSON"));
                _setStatusText(_errorText);
            }
        }

        _setOperation(Operation::Idle);
        reply->deleteLater();
    });
}

void AILogAnalyzeController::_requestLogList(uint32_t start, uint32_t end)
{
    if (!_vehicle) {
        _finishList(false, QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7\uff0c\u65e0\u6cd5\u8bfb\u53d6\u65e5\u5fd7"));
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        _finishList(false, QStringLiteral("\u98de\u63a7\u94fe\u8def\u4e0d\u53ef\u7528\uff0c\u65e0\u6cd5\u8bfb\u53d6\u65e5\u5fd7"));
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_log_request_list_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId(),
        start,
        end
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        _finishList(false, QStringLiteral("\u65e5\u5fd7\u5217\u8868\u8bf7\u6c42\u53d1\u9001\u5931\u8d25"));
        return;
    }

    _timer->start(kRequestLogListTimeoutMs);
}

void AILogAnalyzeController::_requestLogData(uint16_t id, uint32_t offset, uint32_t count)
{
    if (!_vehicle) {
        _finishDownloadWithError(QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7\uff0c\u65e0\u6cd5\u4e0b\u8f7d\u65e5\u5fd7"));
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        _finishDownloadWithError(QStringLiteral("\u98de\u63a7\u94fe\u8def\u4e0d\u53ef\u7528\uff0c\u65e0\u6cd5\u4e0b\u8f7d\u65e5\u5fd7"));
        return;
    }

    id += _apmOffset;
    mavlink_message_t msg{};
    (void) mavlink_msg_log_request_data_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId(),
        id,
        offset,
        count
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        _finishDownloadWithError(QStringLiteral("\u65e5\u5fd7\u6570\u636e\u8bf7\u6c42\u53d1\u9001\u5931\u8d25"));
    }
}

void AILogAnalyzeController::_requestLogEnd()
{
    if (!_vehicle) {
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_log_request_end_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId()
    );

    (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void AILogAnalyzeController::_findMissingEntries()
{
    int start = -1;
    int end = -1;
    const int numLogs = _logEntries->count();
    for (int i = 0; i < numLogs; ++i) {
        const AILogEntry *entry = _logEntries->value<const AILogEntry*>(i);
        if (!entry) {
            continue;
        }
        if (!entry->received()) {
            if (start < 0) {
                start = i;
            } else {
                end = i;
            }
        } else if (start >= 0) {
            break;
        }
    }

    if (start < 0) {
        _finishList(true);
        return;
    }

    if (_retries++ > 2) {
        _finishList(false);
        return;
    }

    if (end < 0) {
        end = start;
    }

    _requestLogList(static_cast<uint32_t>(start + _apmOffset), static_cast<uint32_t>(end + _apmOffset));
}

void AILogAnalyzeController::_findMissingData()
{
    if (!_downloadData) {
        return;
    }

    if (_logComplete()) {
        QByteArray logBytes = _downloadData->bytes;
        const uint logId = _downloadData->id;
        _downloadData.reset();
        _timer->stop();
        _requestLogEnd();
        _uploadLogBytes(logId, logBytes);
        return;
    }

    if (_chunkComplete()) {
        _downloadData->advanceChunk();
    }

    if (_retries++ > 5) {
        _finishDownloadWithError(QStringLiteral("\u65e5\u5fd7\u4e0b\u8f7d\u8d85\u65f6\uff0c\u591a\u6b21\u8865\u5305\u5931\u8d25"));
        return;
    }

    uint16_t start = 0;
    uint16_t end = 0;
    const int size = _downloadData->chunkTable.size();
    for (; start < size; ++start) {
        if (!_downloadData->chunkTable.testBit(start)) {
            break;
        }
    }
    for (end = start; end < size; ++end) {
        if (_downloadData->chunkTable.testBit(end)) {
            break;
        }
    }

    const uint32_t pos = (_downloadData->currentChunk * MemoryDownloadData::kChunkSize) + (start * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    const uint32_t len = (end - start) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
    _requestLogData(_downloadData->id, pos, len);
}

bool AILogAnalyzeController::_entriesComplete() const
{
    const int numLogs = _logEntries->count();
    for (int i = 0; i < numLogs; ++i) {
        const AILogEntry *entry = _logEntries->value<const AILogEntry*>(i);
        if (entry && !entry->received()) {
            return false;
        }
    }
    return true;
}

bool AILogAnalyzeController::_chunkComplete() const
{
    return _downloadData && _downloadData->chunkEquals(true);
}

bool AILogAnalyzeController::_logComplete() const
{
    return _downloadData && _chunkComplete() && ((_downloadData->currentChunk + 1) == _downloadData->numChunks());
}

void AILogAnalyzeController::_startDownload(AILogEntry *entry)
{
    if (!entry || (entry->sizeBytes() == 0)) {
        _finishDownloadWithError(QStringLiteral("\u65e5\u5fd7\u5927\u5c0f\u65e0\u6548，\u65e0\u6cd5\u4e0b\u8f7d"));
        return;
    }
    if (entry->sizeBytes() > static_cast<uint>(std::numeric_limits<int>::max())) {
        _finishDownloadWithError(QStringLiteral("\u65e5\u5fd7\u8fc7\u5927，\u65e0\u6cd5\u5728\u5185\u5b58\u4e2d\u5b89\u5168\u5904\u7406"));
        return;
    }

    if (!entry || entry->sizeBytes() == 0) {
        _setErrorText(QStringLiteral("\u65e5\u5fd7\u5927\u5c0f\u4e3a 0\uff0c\u65e0\u6cd5\u5206\u6790"));
        return;
    }

    _downloadData = std::make_unique<MemoryDownloadData>(entry);
    _downloadData->bytes.resize(static_cast<int>(entry->sizeBytes()));
    _downloadData->bytes.fill('\0');
    _downloadData->currentChunk = 0;
    _downloadData->chunkTable = QBitArray(_downloadData->chunkBins(), false);
    _downloadData->elapsed.start();
    _retries = 0;

    entry->setStatus(QStringLiteral("\u4e0b\u8f7d\u4e2d"));
    _setProgress(0.0);
    _setStatusText(QStringLiteral("\u6b63\u5728\u4e0b\u8f7d\u65e5\u5fd7... 0%"));
    _setOperation(Operation::Downloading);

    _requestLogData(_downloadData->id, 0, _downloadData->chunkTable.size() * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN);
    _timer->start(kTimeoutMs);
}

void AILogAnalyzeController::_startSelectedLogAnalysisAfterBackendReady()
{
    if (_busy) {
        return;
    }

    if (!_vehicle) {
        _setErrorText(QStringLiteral("\u672a\u8fde\u63a5\u98de\u63a7\uff0c\u65e0\u6cd5\u8bfb\u53d6\u65e5\u5fd7"));
        return;
    }

    AILogEntry *entry = _selectedEntry();
    if (!entry) {
        _setErrorText(QStringLiteral("\u8bf7\u5148\u9009\u62e9\u4e00\u6761\u98de\u63a7\u65e5\u5fd7"));
        return;
    }

    _setResultText(QString());
    _setErrorText(QString());
    _setHasResult(false);
    _startDownload(entry);
}

AILogEntry *AILogAnalyzeController::_selectedEntry() const
{
    if ((_selectedLogIndex < 0) || (_selectedLogIndex >= _logEntries->count())) {
        return nullptr;
    }
    return _logEntries->value<AILogEntry*>(_selectedLogIndex);
}

QString AILogAnalyzeController::_localFilePathFromUrl(const QString &filePath)
{
    const QUrl url(filePath);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return filePath;
}

void AILogAnalyzeController::_clearLogEntries()
{
    _logEntries->clearAndDeleteContents();
}
