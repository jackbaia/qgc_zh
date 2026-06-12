/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AudioOutput.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QIODevice>
#include <QtCore/QUrl>
#include <QtCore/qapplicationstatic.h>
#include <QtMultimedia/QSoundEffect>
#include <QtTextToSpeech/QTextToSpeech>

QGC_LOGGING_CATEGORY(AudioOutputLog, "qgc.audio.audiooutput");
// qt.speech.tts.flite
// qt.speech.tts.android

const QHash<QString, QString> AudioOutput::_textHash = {
    { "ERR",            "error" },
    { "POSCTL",         "Position Control" },
    { "ALTCTL",         "Altitude Control" },
    { "AUTO_RTL",       "auto return to launch" },
    { "RTL",            "return To launch" },
    { "ACCEL",          "accelerometer" },
    { "RC_MAP_MODE_SW", "RC mode switch" },
    { "REJ",            "rejected" },
    { "WP",             "waypoint" },
    { "CMD",            "command" },
    { "COMPID",         "component eye dee" },
    { "PARAMS",         "parameters" },
    { "ID",             "I.D." },
    { "ADSB",           "A.D.S.B." },
    { "EKF",            "E.K.F." },
    { "PREARM",         "pre arm" },
    { "PITOT",          "pee toe" },
    { "SERVOX_FUNCTION","Servo X Function" },
};

Q_APPLICATION_STATIC(AudioOutput, _audioOutput);

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    , _engine(new QTextToSpeech(QStringLiteral("none"), this))
    , _audioPackEffect(new QSoundEffect(this))
{
    // qCDebug(AudioOutputLog) << this;
    _audioPackEffect->setLoopCount(1);
}

AudioOutput::~AudioOutput()
{
    // qCDebug(AudioOutputLog) << this;
}

AudioOutput *AudioOutput::instance()
{
    return _audioOutput();
}

void AudioOutput::init(Fact *mutedFact, Fact *voiceStyleFact, Fact *audioPackPathFact)
{
    Q_CHECK_PTR(mutedFact);

    if (_initialized) {
        return;
    }

    if (QTextToSpeech::availableEngines().isEmpty()) {
        qCWarning(AudioOutputLog) << "No available QTextToSpeech engines found.";
        return;
    }

    // Autoselect engine by priority
    if (!_engine->setEngine(QString())) {
        qCWarning(AudioOutputLog) << "Failed to set the TTS engine.";
        return;
    }

    (void) connect(_engine, &QTextToSpeech::engineChanged, this, [this](const QString &engine) {
        qCDebug(AudioOutputLog) << "TTS Engine set to:" << engine;
        const QLocale defaultLocale = QLocale("en_US");
        if (_engine->availableLocales().contains(defaultLocale)) {
            _engine->setLocale(defaultLocale);
        }
    });

    (void) connect(_engine, &QTextToSpeech::aboutToSynthesize, this, [this](qsizetype id) {
        qCDebug(AudioOutputLog) << "TTS About To Synthesize ID:" << id;
        _textQueueSize--;
        qCDebug(AudioOutputLog) << "Queue Size:" << _textQueueSize;
    });

    _voiceStyleFact = voiceStyleFact;
    _audioPackPathFact = audioPackPathFact;

    (void) connect(mutedFact, &Fact::valueChanged, this, [this](QVariant value) {
        setMuted(value.toBool());
    });
    if (_voiceStyleFact) {
        (void) connect(_voiceStyleFact, &Fact::valueChanged, this, [this](QVariant) {
            _applyTtsLocale();
        });
    }
    if (_audioPackPathFact) {
        (void) connect(_audioPackPathFact, &Fact::valueChanged, this, [this](QVariant) {
            _manifestRootPath.clear();
            _manifestEventFiles.clear();
        });
    }
    (void) connect(_audioPackEffect, &QSoundEffect::statusChanged, this, [this]() {
        if (_audioPackEffect->status() == QSoundEffect::Error) {
            qCWarning(AudioOutputLog) << "Audio pack playback error:" << _audioPackEffect->source();
        }
    });

    if (AudioOutputLog().isDebugEnabled()) {
        (void) connect(_engine, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
            qCDebug(AudioOutputLog) << "TTS State changed to:" << state;
        });
        (void) connect(_engine, &QTextToSpeech::errorOccurred, this, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
            qCDebug(AudioOutputLog) << "TTS Error occurred. Reason:" << reason << ", Message:" << errorString;
        });
        (void) connect(_engine, &QTextToSpeech::localeChanged, this, [](const QLocale &locale) {
            qCDebug(AudioOutputLog) << "TTS Locale change to:" << locale;
        });
        (void) connect(_engine, &QTextToSpeech::volumeChanged, this, [](double volume) {
            qCDebug(AudioOutputLog) << "TTS Volume changed to:" << volume;
        });
        (void) connect(_engine, &QTextToSpeech::sayingWord, this, [](const QString &word, qsizetype id, qsizetype start, qsizetype length) {
            qCDebug(AudioOutputLog) << "TTS Saying:" << word << "ID:" << id << "Start:" << start << "Length:" << length;
        });
    }

    setMuted(mutedFact->rawValue().toBool());
    _applyTtsLocale();
    _initialized = true;

    qCDebug(AudioOutputLog) << "AudioOutput initialized with muted state:" << _muted;
}

void AudioOutput::setMuted(bool muted)
{
    if (_muted.exchange(muted) != muted) {
        (void) QMetaObject::invokeMethod(_engine, "setVolume", Qt::AutoConnection, muted ? 0.0 : 1.0);
        _audioPackEffect->setVolume(muted ? 0.0f : 1.0f);
        qCDebug(AudioOutputLog) << "AudioOutput muted state set to:" << muted;
    }
}

void AudioOutput::say(const QString &text, TextMods textMods)
{
    if (!_initialized) {
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using say().";
        }
        return;
    }

    if (_muted) {
        return;
    }

    if (_textQueueSize >= kMaxTextQueueSize) {
        (void) QMetaObject::invokeMethod(_engine, "stop", Qt::AutoConnection, QTextToSpeech::BoundaryHint::Default);
        _audioPackEffect->stop();
        _textQueueSize = 0;
        qCWarning(AudioOutputLog) << "Text queue exceeded maximum size. Stopped current speech.";
    }

    QString outText = _fixTextMessageForAudio(text);

    if (textMods.testFlag(TextMod::Translate)) {
        outText = tr("%1").arg(outText);
    }

    if (_voiceStyle() == DongbeiAudioPack) {
        const QString eventKey = _detectAudioEventKey(outText);
        if (!eventKey.isEmpty() && _playAudioPackEvent(eventKey)) {
            return;
        }
    }

    if (!_engine->engineCapabilities().testFlag(QTextToSpeech::Capability::Speak)) {
        qCWarning(AudioOutputLog) << "Speech Not Supported:" << text;
        return;
    }

    _applyTtsLocale();

    qsizetype index;
    if (QMetaObject::invokeMethod(_engine, "enqueue", Qt::AutoConnection, qReturnArg(index), outText)) {
        if (index != -1) {
            _textQueueSize++;
            qCDebug(AudioOutputLog) << "Enqueued text with index:" << index << ", Queue Size:" << _textQueueSize;
        }
    } else {
        qCWarning(AudioOutputLog) << "Failed to invoke Enqueue method.";
    }
}

bool AudioOutput::playAudioPackEventForTest(const QString &eventKey)
{
    if (_muted) {
        return false;
    }

    return _playAudioPackEvent(eventKey);
}

QString AudioOutput::_fixTextMessageForAudio(const QString &string)
{
    QString result = string;
    result = _replaceAbbreviations(result);
    result = _replaceNegativeSigns(result);
    result = _replaceDecimalPoints(result);
    result = _replaceMeters(result);
    result = _convertMilliseconds(result);
    return result;
}

QString AudioOutput::_replaceAbbreviations(const QString &input)
{
    QString output = input;

    const QStringList wordList = input.split(' ', Qt::SkipEmptyParts);
    for (const QString &word : wordList) {
        const QString upperWord = word.toUpper();
        if (_textHash.contains(upperWord)) {
            (void) output.replace(word, _textHash.value(upperWord));
        }
    }

    return output;
}

QString AudioOutput::_replaceNegativeSigns(const QString &input)
{
    static const QRegularExpression negNumRegex(QStringLiteral("-\\s*(?=\\d)"));
    Q_ASSERT(negNumRegex.isValid());

    QString output = input;
    (void) output.replace(negNumRegex, "negative ");
    return output;
}

QString AudioOutput::_replaceDecimalPoints(const QString &input)
{
    static const QRegularExpression realNumRegex(QStringLiteral("([0-9]+)(\\.)([0-9]+)"));
    Q_ASSERT(realNumRegex.isValid());

    QString output = input;
    QRegularExpressionMatch realNumRegexMatch = realNumRegex.match(output);
    while (realNumRegexMatch.hasMatch()) {
        if (!realNumRegexMatch.captured(2).isNull()) {
            (void) output.replace(realNumRegexMatch.capturedStart(2), realNumRegexMatch.capturedEnd(2) - realNumRegexMatch.capturedStart(2), QStringLiteral(" point "));
        }
        realNumRegexMatch = realNumRegex.match(output);
    }

    return output;
}

QString AudioOutput::_replaceMeters(const QString &input)
{
    static const QRegularExpression realNumMeterRegex(QStringLiteral("[0-9]*\\.?[0-9]\\s?(m)([^A-Za-z]|$)"));
    Q_ASSERT(realNumMeterRegex.isValid());

    QString output = input;
    QRegularExpressionMatch realNumMeterRegexMatch = realNumMeterRegex.match(output);
    while (realNumMeterRegexMatch.hasMatch()) {
        if (!realNumMeterRegexMatch.captured(1).isNull()) {
            (void) output.replace(realNumMeterRegexMatch.capturedStart(1), realNumMeterRegexMatch.capturedEnd(1) - realNumMeterRegexMatch.capturedStart(1), QStringLiteral(" meters"));
        }
        realNumMeterRegexMatch = realNumMeterRegex.match(output);
    }

    return output;
}

QString AudioOutput::_convertMilliseconds(const QString &input)
{
    QString result = input;

    QString match;
    int number;
    if (_getMillisecondString(input, match, number) && (number >= 1000)) {
        QString newNumber;
        if (number < 60000) {
            const int seconds = number / 1000;
            const int ms = number - (seconds * 1000);
            newNumber = QStringLiteral("%1 second%2").arg(seconds).arg(seconds > 1 ? "s" : "");
            if (ms > 0) {
                (void) newNumber.append(QStringLiteral(" and %1 millisecond").arg(ms));
            }
        } else {
            const int minutes = number / 60000;
            const int seconds = (number - (minutes * 60000)) / 1000;
            newNumber = QStringLiteral("%1 minute%2").arg(minutes).arg(minutes > 1 ? "s" : "");
            if (seconds > 0) {
                (void) newNumber.append(QStringLiteral(" and %1 second%2").arg(seconds).arg(seconds > 1 ? "s" : ""));
            }
        }
        (void) result.replace(match, newNumber);
    }

    return result;
}

bool AudioOutput::_getMillisecondString(const QString &string, QString &match, int &number)
{
    static const QRegularExpression msRegex("((?<number>[0-9]+)ms)");
    Q_ASSERT(msRegex.isValid());

    bool result = false;

    QRegularExpressionMatch regexpMatch = msRegex.match(string);
    if (regexpMatch.hasMatch()) {
        match = regexpMatch.captured(0);
        const QString numberStr = regexpMatch.captured("number");
        number = numberStr.toInt();
        result = true;
    }

    return result;
}

int AudioOutput::_voiceStyle() const
{
    return _voiceStyleFact ? _voiceStyleFact->rawValue().toInt() : SystemTTS;
}

void AudioOutput::_applyTtsLocale()
{
    if (!_engine) {
        return;
    }

    const int style = _voiceStyle();
    if ((style == ChineseTTS) || (style == DongbeiAudioPack)) {
        const QLocale chineseLocale(QLocale::Chinese, QLocale::SimplifiedChineseScript, QLocale::China);
        if (_engine->availableLocales().contains(chineseLocale) && (_engine->locale() != chineseLocale)) {
            _engine->setLocale(chineseLocale);
        }
    }
}

bool AudioOutput::_playAudioPackEvent(const QString &eventKey)
{
    const QString audioFile = _audioFileForEvent(eventKey);
    if (audioFile.isEmpty() || !QFileInfo::exists(audioFile)) {
        qCDebug(AudioOutputLog) << "Audio pack file not found for event:" << eventKey << audioFile;
        return false;
    }

    _audioPackEffect->stop();
    _audioPackEffect->setSource(QUrl::fromLocalFile(audioFile));
    if (_audioPackEffect->status() == QSoundEffect::Error) {
        qCWarning(AudioOutputLog) << "Audio pack file could not be loaded:" << audioFile;
        return false;
    }

    _audioPackEffect->play();
    qCDebug(AudioOutputLog) << "Playing audio pack event:" << eventKey << audioFile;
    return true;
}

QString AudioOutput::_detectAudioEventKey(const QString &text) const
{
    QString normalized = text.toLower();
    normalized.replace('_', ' ');
    normalized.replace('-', ' ');

    const auto hasAny = [&normalized](std::initializer_list<const char*> words) {
        for (const char *word: words) {
            if (normalized.contains(QString::fromUtf8(word))) {
                return true;
            }
        }
        return false;
    };

    if (hasAny({"emergency kill", "kill switch", "flight termination", "紧急停机", "急停"})) {
        return QStringLiteral("emergency_kill");
    }
    if (hasAny({"prearm", "pre arm", "preflight check", "arming check", "解锁检查", "预解锁"}) &&
            hasAny({"fail", "failed", "denied", "reject", "失败", "未通过", "拒绝"})) {
        return QStringLiteral("prearm_failed");
    }
    if (hasAny({"battery critical", "critical battery", "critical low battery", "电池严重", "严重低电量"})) {
        return QStringLiteral("battery_critical");
    }
    if (hasAny({"battery low", "low battery", "low voltage", "电池电量低", "低电量", "低电压"})) {
        return QStringLiteral("battery_low");
    }
    if (hasAny({"gps regained", "gps recovered", "gps signal restored", "gps 恢复", "gps信号恢复"})) {
        return QStringLiteral("gps_recovered");
    }
    if (hasAny({"gps lost", "gps failure", "gps signal lost", "no gps", "gps 丢失", "gps信号丢失", "gps 信号丢失"})) {
        return QStringLiteral("gps_lost");
    }
    if (hasAny({"radio regained", "rc regained", "rc recovered", "manual control regained", "遥控恢复", "遥控信号恢复"})) {
        return QStringLiteral("rc_recovered");
    }
    if (hasAny({"radio lost", "rc lost", "manual control lost", "communication lost", "遥控丢失", "遥控信号丢失", "通信丢失"})) {
        return QStringLiteral("rc_lost");
    }
    if (hasAny({"failsafe", "fail safe", "失效保护", "故障保护"})) {
        return QStringLiteral("failsafe");
    }
    if (hasAny({"ekf", "estimator", "innovation", "状态估计"})) {
        return QStringLiteral("ekf_warning");
    }
    if (hasAny({"compass", "mag", "magnetometer", "磁罗盘", "磁力计"})) {
        return QStringLiteral("compass_error");
    }
    if (hasAny({"airspeed", "pitot", "空速"})) {
        return QStringLiteral("airspeed_warning");
    }
    if (hasAny({"takeoff", "take off", "起飞"})) {
        return QStringLiteral("takeoff");
    }
    if (hasAny({"landing", "land", "降落", "着陆"})) {
        return QStringLiteral("landing");
    }
    if (hasAny({"log analysis started", "开始分析飞行日志", "日志分析开始"})) {
        return QStringLiteral("log_analysis_started");
    }
    if (hasAny({"log analysis finished", "日志分析完成", "完成分析飞行日志"})) {
        return QStringLiteral("log_analysis_finished");
    }
    if (hasAny({"return to launch", "rtl", "返航"})) {
        return QStringLiteral("mode_rtl");
    }
    if (hasAny({"offboard", "external control", "外部控制"})) {
        return QStringLiteral("mode_offboard");
    }
    if (hasAny({"mission flight mode", "mission mode", "任务模式", "航线任务"})) {
        return QStringLiteral("mode_mission");
    }
    if (hasAny({"position control", "position flight mode", "position mode", "位置模式"})) {
        return QStringLiteral("mode_position");
    }
    if (hasAny({"altitude control", "altitude flight mode", "altitude mode", "高度模式"})) {
        return QStringLiteral("mode_altitude");
    }
    if (hasAny({"stabilized", "stabilize", "姿态模式", "自稳"})) {
        return QStringLiteral("mode_stabilized");
    }
    if (hasAny({"manual flight mode", "manual mode", "手动模式"})) {
        return QStringLiteral("mode_manual");
    }
    if (hasAny({"disarmed", "disarm", "上锁", "锁定电机"})) {
        return QStringLiteral("disarm");
    }
    if (hasAny({"armed", "arm success", "解锁成功", "已解锁"})) {
        return QStringLiteral("arm_success");
    }

    return QString();
}

QString AudioOutput::_audioPackRootPath() const
{
    const QString configuredPath = _audioPackPathFact ? _audioPackPathFact->rawValue().toString() : QString();
    if (!configuredPath.isEmpty()) {
        const QUrl url(configuredPath);
        return url.isLocalFile() ? url.toLocalFile() : configuredPath;
    }

    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("audio/dongbei"));
}

QString AudioOutput::_audioFileForEvent(const QString &eventKey)
{
    const QString rootPath = _audioPackRootPath();
    if (!_loadAudioPackManifest(rootPath)) {
        return QDir(rootPath).filePath(eventKey + QStringLiteral(".wav"));
    }

    const QString manifestFile = _manifestEventFiles.value(eventKey);
    if (!manifestFile.isEmpty()) {
        return QDir(rootPath).filePath(manifestFile);
    }

    return QDir(rootPath).filePath(eventKey + QStringLiteral(".wav"));
}

bool AudioOutput::_loadAudioPackManifest(const QString &rootPath)
{
    if (_manifestRootPath == rootPath) {
        return !_manifestEventFiles.isEmpty();
    }

    _manifestRootPath = rootPath;
    _manifestEventFiles.clear();

    QFile manifestFile(QDir(rootPath).filePath(QStringLiteral("manifest.json")));
    if (!manifestFile.exists()) {
        return false;
    }
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        qCWarning(AudioOutputLog) << "Failed to open audio pack manifest:" << manifestFile.fileName();
        return false;
    }

    const QJsonDocument manifest = QJsonDocument::fromJson(manifestFile.readAll());
    if (!manifest.isObject()) {
        qCWarning(AudioOutputLog) << "Invalid audio pack manifest:" << manifestFile.fileName();
        return false;
    }

    const QJsonObject events = manifest.object().value(QStringLiteral("events")).toObject();
    for (auto it = events.begin(); it != events.end(); ++it) {
        const QJsonObject event = it.value().toObject();
        const QString fileName = event.value(QStringLiteral("file")).toString();
        if (!fileName.isEmpty()) {
            _manifestEventFiles.insert(it.key(), fileName);
        }
    }

    return !_manifestEventFiles.isEmpty();
}
