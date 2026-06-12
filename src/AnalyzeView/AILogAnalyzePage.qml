/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Controllers

AnalyzePage {
    pageComponent: pageComponent
    pageName: qsTr("AI \u65e5\u5fd7\u5206\u6790")
    pageDescription: qsTr("\u4ece\u5f53\u524d\u8fde\u63a5\u7684\u98de\u63a7\u8bfb\u53d6 onboard logs\uff0c\u4e0b\u8f7d\u5230\u5185\u5b58\u540e\u4e0a\u4f20\u672c\u5730 ULog \u5206\u6790\u670d\u52a1\u5e76\u663e\u793a JSON \u7ed3\u679c\u3002")

    readonly property real _margin: ScreenTools.defaultFontPixelWidth
    readonly property real _buttonWidth: ScreenTools.defaultFontPixelWidth * 16

    AILogAnalyzeController {
        id: controller
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: true
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width: availableWidth
            height: availableHeight
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel {
                text: qsTr("AI \u65e5\u5fd7\u5206\u6790")
                font.pointSize: ScreenTools.largeFontPointSize
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: _margin

                QGCButton {
                    text: qsTr("\u5237\u65b0\u65e5\u5fd7\u5217\u8868")
                    enabled: !controller.busy
                    Layout.preferredWidth: _buttonWidth
                    onClicked: controller.refreshLogList()
                }

                QGCButton {
                    text: controller.busy ? qsTr("\u5904\u7406\u4e2d...") : qsTr("\u8bfb\u53d6\u5e76\u5206\u6790")
                    enabled: !controller.busy && controller.selectedLogIndex >= 0
                    Layout.preferredWidth: _buttonWidth
                    onClicked: controller.analyzeSelectedLog()
                }

                QGCButton {
                    text: qsTr("\u4e0b\u8f7d JSON \u6587\u4ef6")
                    enabled: controller.hasResult && !controller.busy
                    Layout.preferredWidth: _buttonWidth + ScreenTools.defaultFontPixelWidth * 4
                    onClicked: saveJsonFile.openForSave()

                    QGCFileDialog {
                        id: saveJsonFile
                        title: qsTr("\u4fdd\u5b58 JSON \u6587\u4ef6")
                        nameFilters: [qsTr("JSON files (*.json)")]
                        defaultSuffix: "json"
                        onAcceptedForSave: (file) => {
                            controller.saveResultJson(file)
                            close()
                        }
                    }
                }
            }

            ProgressBar {
                from: 0
                to: 1
                value: controller.progress
                visible: controller.busy
                Layout.fillWidth: true
            }

            QGCLabel {
                text: controller.statusText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            QGCLabel {
                text: qsTr("ULog \u5206\u6790\u670d\u52a1\uff1a") + controller.backendStatusText
                color: controller.backendRunning ? qgcPal.text : qgcPal.warningText
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            QGCLabel {
                text: controller.errorText
                color: qgcPal.warningText
                visible: controller.errorText.length > 0
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(ScreenTools.defaultFontPixelHeight * 14, availableHeight * 0.35)
                color: qgcPal.windowShade
                border.color: qgcPal.text
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel { text: "ID"; Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6; font.bold: true }
                        QGCLabel { text: qsTr("\u65f6\u95f4"); Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 24; font.bold: true }
                        QGCLabel { text: qsTr("\u5927\u5c0f"); Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 12; font.bold: true }
                        QGCLabel { text: qsTr("\u72b6\u6001"); Layout.fillWidth: true; font.bold: true }
                    }

                    ListView {
                        id: logListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: controller.logEntries
                        clip: true

                        delegate: Rectangle {
                            width: logListView.width
                            height: ScreenTools.defaultFontPixelHeight * 2
                            color: controller.selectedLogIndex === index ? qgcPal.buttonHighlight : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                                anchors.rightMargin: ScreenTools.defaultFontPixelWidth / 2

                                QGCLabel {
                                    text: object.logId
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                                }
                                QGCLabel {
                                    text: object.timeUtc && object.timeUtc.getUTCFullYear() >= 2010 ? object.timeUtc.toLocaleString(undefined) : "UnknownDate"
                                    elide: Text.ElideRight
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 24
                                }
                                QGCLabel {
                                    text: object.sizeBytes
                                    elide: Text.ElideRight
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 12
                                }
                                QGCLabel {
                                    text: object.status
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !controller.busy
                                onClicked: controller.selectedLogIndex = index
                            }
                        }
                    }
                }
            }

            QGCFlickable {
                id: resultFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: resultTextArea.width
                contentHeight: resultTextArea.height
                clip: true

                TextArea.flickable: TextArea {
                    id: resultTextArea
                    width: resultFlickable.width
                    text: controller.resultText
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    color: qgcPal.text
                    selectedTextColor: qgcPal.windowShade
                    selectionColor: qgcPal.text
                    font.pointSize: ScreenTools.defaultFontPointSize
                    font.family: ScreenTools.fixedFontFamily
                    placeholderText: qsTr("\u540e\u7aef\u8fd4\u56de\u7684 JSON \u5185\u5bb9\u5c06\u663e\u793a\u5728\u8fd9\u91cc\u3002")
                    background: Rectangle {
                        color: qgcPal.windowShade
                    }
                }
            }
        }
    }
}

