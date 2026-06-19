/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

namespace ChineseMessageTranslator
{

static inline bool _containsEnglishWord(const QString &text)
{
    static const QRegularExpression englishWord(QStringLiteral("[A-Za-z]{4,}"));
    return englishWord.match(text).hasMatch();
}

static inline void _replace(QString &text, const QString &from, const QString &to)
{
    text.replace(from, to, Qt::CaseInsensitive);
}

static inline QString explain(QString text)
{
    if (text.trimmed().isEmpty()) {
        return text;
    }

    _replace(text, QStringLiteral("Preflight Fail:"), QStringLiteral("\u8d77\u98de\u524d\u68c0\u67e5\u5931\u8d25:"));
    _replace(text, QStringLiteral("Preflight check failed"), QStringLiteral("\u8d77\u98de\u524d\u68c0\u67e5\u672a\u901a\u8fc7"));
    _replace(text, QStringLiteral("PreArm:"), QStringLiteral("\u89e3\u9501\u524d\u68c0\u67e5:"));
    _replace(text, QStringLiteral("PreArm"), QStringLiteral("\u89e3\u9501\u524d\u68c0\u67e5"));
    _replace(text, QStringLiteral("Arming denied"), QStringLiteral("\u62d2\u7edd\u89e3\u9501"));
    _replace(text, QStringLiteral("Arm denied"), QStringLiteral("\u62d2\u7edd\u89e3\u9501"));
    _replace(text, QStringLiteral("Disarming denied"), QStringLiteral("\u62d2\u7edd\u4e0a\u9501"));
    _replace(text, QStringLiteral("Takeoff denied"), QStringLiteral("\u62d2\u7edd\u8d77\u98de"));
    _replace(text, QStringLiteral("Mission rejected"), QStringLiteral("\u4efb\u52a1\u88ab\u62d2\u7edd"));
    _replace(text, QStringLiteral("Mission start denied"), QStringLiteral("\u62d2\u7edd\u5f00\u59cb\u4efb\u52a1"));
    _replace(text, QStringLiteral("Command denied"), QStringLiteral("\u547d\u4ee4\u88ab\u62d2\u7edd"));
    _replace(text, QStringLiteral("Command failed"), QStringLiteral("\u547d\u4ee4\u6267\u884c\u5931\u8d25"));
    _replace(text, QStringLiteral("Command unsupported"), QStringLiteral("\u4e0d\u652f\u6301\u8be5\u547d\u4ee4"));
    _replace(text, QStringLiteral("temporarily rejected"), QStringLiteral("\u6682\u65f6\u62d2\u7edd"));

    _replace(text, QStringLiteral("No valid local position estimate"), QStringLiteral("\u672c\u5730\u4f4d\u7f6e\u4f30\u8ba1\u65e0\u6548"));
    _replace(text, QStringLiteral("No valid global position estimate"), QStringLiteral("\u5168\u5c40\u4f4d\u7f6e\u4f30\u8ba1\u65e0\u6548"));
    _replace(text, QStringLiteral("No valid position estimate"), QStringLiteral("\u4f4d\u7f6e\u4f30\u8ba1\u65e0\u6548"));
    _replace(text, QStringLiteral("The available positioning data is not sufficient to execute the selected mode."),
        QStringLiteral("\u5f53\u524d\u53ef\u7528\u7684\u5b9a\u4f4d\u6570\u636e\u4e0d\u8db3\uff0c\u65e0\u6cd5\u6267\u884c\u6240\u9009\u6a21\u5f0f\u3002"));
    _replace(text, QStringLiteral("No manual control input"), QStringLiteral("\u6ca1\u6709\u624b\u52a8\u63a7\u5236\u8f93\u5165"));
    _replace(text, QStringLiteral("Connect and enable stick input or use autonomous mode."),
        QStringLiteral("\u8bf7\u8fde\u63a5\u5e76\u542f\u7528\u9065\u6746\u8f93\u5165\uff0c\u6216\u4f7f\u7528\u81ea\u4e3b\u98de\u884c\u6a21\u5f0f\u3002"));
    _replace(text, QStringLiteral("Sticks can be enabled via"),
        QStringLiteral("\u53ef\u901a\u8fc7"));
    _replace(text, QStringLiteral("parameter."), QStringLiteral("\u53c2\u6570\u542f\u7528\u9065\u6746\u8f93\u5165\u3002"));
    _replace(text, QStringLiteral("parameters."), QStringLiteral("\u53c2\u6570\u542f\u7528\u9065\u6746\u8f93\u5165\u3002"));
    _replace(text, QStringLiteral("Navigation"), QStringLiteral("\u5bfc\u822a"));

    _replace(text, QStringLiteral("No global position"), QStringLiteral("\u6ca1\u6709\u5168\u5c40\u4f4d\u7f6e"));
    _replace(text, QStringLiteral("No local position"), QStringLiteral("\u6ca1\u6709\u672c\u5730\u4f4d\u7f6e"));
    _replace(text, QStringLiteral("No valid local position"), QStringLiteral("\u672c\u5730\u4f4d\u7f6e\u65e0\u6548"));
    _replace(text, QStringLiteral("No valid global position"), QStringLiteral("\u5168\u5c40\u4f4d\u7f6e\u65e0\u6548"));
    _replace(text, QStringLiteral("positioning data"), QStringLiteral("\u5b9a\u4f4d\u6570\u636e"));
    _replace(text, QStringLiteral("position estimate"), QStringLiteral("\u4f4d\u7f6e\u4f30\u8ba1"));
    _replace(text, QStringLiteral("position lock"), QStringLiteral("\u4f4d\u7f6e\u9501\u5b9a"));
    _replace(text, QStringLiteral("Home position not set"), QStringLiteral("Home \u70b9\u672a\u8bbe\u7f6e"));
    _replace(text, QStringLiteral("home position unknown"), QStringLiteral("Home \u70b9\u672a\u77e5"));

    _replace(text, QStringLiteral("GPS signal lost"), QStringLiteral("GPS \u4fe1\u53f7\u4e22\u5931"));
    _replace(text, QStringLiteral("GPS failure"), QStringLiteral("GPS \u6545\u969c"));
    _replace(text, QStringLiteral("GPS fix"), QStringLiteral("GPS \u5b9a\u4f4d"));
    _replace(text, QStringLiteral("No GPS"), QStringLiteral("\u672a\u68c0\u6d4b\u5230 GPS"));
    _replace(text, QStringLiteral("RTK"), QStringLiteral("RTK"));
    _replace(text, QStringLiteral("Compass not calibrated"), QStringLiteral("\u78c1\u7f57\u76d8\u672a\u6821\u51c6"));
    _replace(text, QStringLiteral("Compass inconsistent"), QStringLiteral("\u78c1\u7f57\u76d8\u6570\u636e\u4e0d\u4e00\u81f4"));
    _replace(text, QStringLiteral("mag sensors inconsistent"), QStringLiteral("\u78c1\u529b\u8ba1\u6570\u636e\u4e0d\u4e00\u81f4"));
    _replace(text, QStringLiteral("accel sensors inconsistent"), QStringLiteral("\u52a0\u901f\u5ea6\u8ba1\u6570\u636e\u4e0d\u4e00\u81f4"));
    _replace(text, QStringLiteral("gyro sensors inconsistent"), QStringLiteral("\u9640\u87ba\u4eea\u6570\u636e\u4e0d\u4e00\u81f4"));
    _replace(text, QStringLiteral("Accelerometer not calibrated"), QStringLiteral("\u52a0\u901f\u5ea6\u8ba1\u672a\u6821\u51c6"));
    _replace(text, QStringLiteral("Gyro not calibrated"), QStringLiteral("\u9640\u87ba\u4eea\u672a\u6821\u51c6"));
    _replace(text, QStringLiteral("Baro not calibrated"), QStringLiteral("\u6c14\u538b\u8ba1\u672a\u6821\u51c6"));
    _replace(text, QStringLiteral("Airspeed sensor missing"), QStringLiteral("\u7a7a\u901f\u4f20\u611f\u5668\u7f3a\u5931"));
    _replace(text, QStringLiteral("Airspeed invalid"), QStringLiteral("\u7a7a\u901f\u6570\u636e\u65e0\u6548"));
    _replace(text, QStringLiteral("Sensor failure"), QStringLiteral("\u4f20\u611f\u5668\u6545\u969c"));
    _replace(text, QStringLiteral("sensors inconsistent"), QStringLiteral("\u4f20\u611f\u5668\u6570\u636e\u4e0d\u4e00\u81f4"));
    _replace(text, QStringLiteral("High Accelerometer Bias"), QStringLiteral("\u52a0\u901f\u5ea6\u8ba1\u96f6\u504f\u8fc7\u5927"));
    _replace(text, QStringLiteral("High Gyro Bias"), QStringLiteral("\u9640\u87ba\u4eea\u96f6\u504f\u8fc7\u5927"));

    _replace(text, QStringLiteral("RC calibration"), QStringLiteral("\u9065\u63a7\u5668\u6821\u51c6"));
    _replace(text, QStringLiteral("RC not found"), QStringLiteral("\u672a\u68c0\u6d4b\u5230\u9065\u63a7\u5668"));
    _replace(text, QStringLiteral("RC signal lost"), QStringLiteral("\u9065\u63a7\u4fe1\u53f7\u4e22\u5931"));
    _replace(text, QStringLiteral("Manual control lost"), QStringLiteral("\u624b\u52a8\u63a7\u5236\u4fe1\u53f7\u4e22\u5931"));

    _replace(text, QStringLiteral("Battery warning"), QStringLiteral("\u7535\u6c60\u544a\u8b66"));
    _replace(text, QStringLiteral("Critical battery"), QStringLiteral("\u4e25\u91cd\u4f4e\u7535\u91cf"));
    _replace(text, QStringLiteral("Emergency battery"), QStringLiteral("\u7d27\u6025\u4f4e\u7535\u91cf"));
    _replace(text, QStringLiteral("Low battery"), QStringLiteral("\u4f4e\u7535\u91cf"));
    _replace(text, QStringLiteral("Battery unhealthy"), QStringLiteral("\u7535\u6c60\u72b6\u6001\u5f02\u5e38"));
    _replace(text, QStringLiteral("Battery missing"), QStringLiteral("\u672a\u68c0\u6d4b\u5230\u7535\u6c60"));
    _replace(text, QStringLiteral("Battery low"), QStringLiteral("\u7535\u91cf\u4f4e"));
    _replace(text, QStringLiteral("Power module"), QStringLiteral("\u7535\u6e90\u6a21\u5757"));

    _replace(text, QStringLiteral("Data link lost"), QStringLiteral("\u6570\u4f20\u94fe\u8def\u4e22\u5931"));
    _replace(text, QStringLiteral("Failsafe enabled"), QStringLiteral("\u5df2\u89e6\u53d1\u6545\u969c\u4fdd\u62a4"));
    _replace(text, QStringLiteral("failsafe"), QStringLiteral("\u6545\u969c\u4fdd\u62a4"));
    _replace(text, QStringLiteral("Geofence violation"), QStringLiteral("\u89e6\u53d1\u5730\u7406\u56f4\u680f"));
    _replace(text, QStringLiteral("Geofence breach"), QStringLiteral("\u7a81\u7834\u5730\u7406\u56f4\u680f"));
    _replace(text, QStringLiteral("Return to launch"), QStringLiteral("\u8fd4\u822a"));
    _replace(text, QStringLiteral("RTL"), QStringLiteral("\u8fd4\u822a"));
    _replace(text, QStringLiteral("Landing"), QStringLiteral("\u964d\u843d"));
    _replace(text, QStringLiteral("Takeoff"), QStringLiteral("\u8d77\u98de"));
    _replace(text, QStringLiteral("Mission"), QStringLiteral("\u4efb\u52a1"));
    _replace(text, QStringLiteral("Offboard lost"), QStringLiteral("Offboard \u63a7\u5236\u4e22\u5931"));

    _replace(text, QStringLiteral("Estimator"), QStringLiteral("\u4f30\u8ba1\u5668"));
    _replace(text, QStringLiteral("estimator"), QStringLiteral("\u4f30\u8ba1\u5668"));
    _replace(text, QStringLiteral("heading"), QStringLiteral("\u822a\u5411"));
    _replace(text, QStringLiteral("attitude"), QStringLiteral("\u59ff\u6001"));
    _replace(text, QStringLiteral("velocity"), QStringLiteral("\u901f\u5ea6"));
    _replace(text, QStringLiteral("position"), QStringLiteral("\u4f4d\u7f6e"));
    _replace(text, QStringLiteral("altitude"), QStringLiteral("\u9ad8\u5ea6"));
    _replace(text, QStringLiteral("parameter"), QStringLiteral("\u53c2\u6570"));
    _replace(text, QStringLiteral("calibration"), QStringLiteral("\u6821\u51c6"));
    _replace(text, QStringLiteral("timeout"), QStringLiteral("\u8d85\u65f6"));
    _replace(text, QStringLiteral("timed out"), QStringLiteral("\u8d85\u65f6"));
    _replace(text, QStringLiteral("invalid"), QStringLiteral("\u65e0\u6548"));
    _replace(text, QStringLiteral("missing"), QStringLiteral("\u7f3a\u5931"));
    _replace(text, QStringLiteral("required"), QStringLiteral("\u5fc5\u9700"));
    _replace(text, QStringLiteral("not set"), QStringLiteral("\u672a\u8bbe\u7f6e"));
    _replace(text, QStringLiteral("not valid"), QStringLiteral("\u65e0\u6548"));
    _replace(text, QStringLiteral("failed"), QStringLiteral("\u5931\u8d25"));
    _replace(text, QStringLiteral("failure"), QStringLiteral("\u6545\u969c"));
    _replace(text, QStringLiteral("rejected"), QStringLiteral("\u88ab\u62d2\u7edd"));
    _replace(text, QStringLiteral("denied"), QStringLiteral("\u88ab\u62d2\u7edd"));
    _replace(text, QStringLiteral("unsupported"), QStringLiteral("\u4e0d\u652f\u6301"));
    _replace(text, QStringLiteral("not supported"), QStringLiteral("\u4e0d\u652f\u6301"));
    _replace(text, QStringLiteral("warning"), QStringLiteral("\u544a\u8b66"));
    _replace(text, QStringLiteral("error"), QStringLiteral("\u9519\u8bef"));

    const QRegularExpression preflightRegex(QStringLiteral("^\\s*\\[?preflight\\]?\\s*[:\\-]?\\s*"), QRegularExpression::CaseInsensitiveOption);
    text.replace(preflightRegex, QStringLiteral("\u8d77\u98de\u524d\u68c0\u67e5: "));

    const QRegularExpression paramMissingRegex(QStringLiteral("\\b([A-Z][A-Z0-9_]{2,})\\s+缺失"), QRegularExpression::CaseInsensitiveOption);
    text.replace(paramMissingRegex, QStringLiteral("\u7f3a\u5931\u53c2\u6570 \\1"));

    const QString flightControllerReportPrefix = QStringLiteral("\u98de\u63a7\u62a5\u544a: ");
    if (!text.startsWith(flightControllerReportPrefix) && _containsEnglishWord(text)) {
        text = flightControllerReportPrefix + text;
    }

    return text;
}

}
