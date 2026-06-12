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
#include <QtCore/QHash>

class QTextToSpeech;
class QSoundEffect;
class Fact;
class AudioOutputTest;

Q_DECLARE_LOGGING_CATEGORY(AudioOutputLog)

/// The AudioOutput class provides functionality for audio output using text-to-speech.
class AudioOutput : public QObject
{
    Q_OBJECT

    friend AudioOutputTest;

public:
    /// Enumeration for text modification options.
    enum class TextMod {
        None = 0,
        Translate = 1 << 0,
    };
    Q_FLAG(TextMod)
    Q_DECLARE_FLAGS(TextMods, TextMod)

    /// Constructs an AudioOutput object.
    ///     @param parent The parent QObject.
    explicit AudioOutput(QObject *parent = nullptr);

    /// Destructor for the AudioOutput class.
    ~AudioOutput();

    /// Gets the singleton instance of AudioOutput.
    ///     @return The singleton instance.
    static AudioOutput *instance();

    /// Initialize the Singleton
    void init(Fact *mutedFact, Fact *voiceStyleFact = nullptr, Fact *audioPackPathFact = nullptr);

    /// Checks if the audio output is muted.
    ///     @return True if muted, false otherwise.
    bool isMuted() const { return _muted; }

    /// Sets the mute state of the audio output.
    ///     @param enable True to mute, false to unmute.
    void setMuted(bool muted);

    /// Reads the specified text with optional text modifications.
    ///     @param text The text to be read.
    ///     @param textMods The text modifications to apply.
    void say(const QString &text, TextMods textMods = TextMod::None);
    bool playAudioPackEventForTest(const QString &eventKey);

private:
    enum VoiceStyle {
        SystemTTS = 0,
        ChineseTTS = 1,
        DongbeiAudioPack = 2,
    };

    QTextToSpeech *_engine = nullptr;
    QSoundEffect *_audioPackEffect = nullptr;
    Fact *_voiceStyleFact = nullptr;
    Fact *_audioPackPathFact = nullptr;
    QAtomicInteger<qsizetype> _textQueueSize = 0;
    bool _initialized = false;
    std::atomic_bool _muted = false;
    QString _manifestRootPath;
    QHash<QString, QString> _manifestEventFiles;

    static const QHash<QString, QString> _textHash;

    static constexpr qsizetype kMaxTextQueueSize = 20;

    /// Fixes text messages for audio output.
    ///     @param string The text message to fix.
    ///     @return The fixed text message.
    static QString _fixTextMessageForAudio(const QString &string);

    /// Replaces predefined abbreviations with their corresponding full forms.
    ///     @param input The input string containing abbreviations.
    ///     @return A string with abbreviations replaced by their full forms.
    static QString _replaceAbbreviations(const QString &input);

    /// Replaces negative signs with the word "negative".
    static QString _replaceNegativeSigns(const QString &input);

    /// Replaces decimal points with the word "point".
    static QString _replaceDecimalPoints(const QString &input);

    /// Replaces "m" (meters) with the word "meters" following numbers.
    static QString _replaceMeters(const QString &input);

    /// Converts millisecond values to a more readable format (seconds and minutes).
    static QString _convertMilliseconds(const QString &input);

    /// Extracts a millisecond value from the given string.
    ///     @param string The string to extract from.
    ///     @param match The extracted millisecond string.
    ///     @param number The extracted number.
    ///     @return True if extraction is successful, false otherwise.
    static bool _getMillisecondString(const QString &string, QString &match, int &number);

    int _voiceStyle() const;
    void _applyTtsLocale();
    bool _playAudioPackEvent(const QString &eventKey);
    QString _detectAudioEventKey(const QString &text) const;
    QString _audioPackRootPath() const;
    QString _audioFileForEvent(const QString &eventKey);
    bool _loadAudioPackManifest(const QString &rootPath);

};
Q_DECLARE_OPERATORS_FOR_FLAGS(AudioOutput::TextMods)
