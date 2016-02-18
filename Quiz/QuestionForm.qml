import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

//import "vars.js" as Vars

Item {
    id: root
    width: 640
    height: 640
    property int currentQuiz: (parent === null) ? -1 : parent.currentQuiz;

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        RowLayout {
            anchors.fill: parent
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Text {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                text: client.model.getName(currentQuiz)
                font.pointSize: 14
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                text: "Question " + (client.model.getPosition(currentQuiz) + 1) + "/" + client.model.getTotal(currentQuiz)
                font.pointSize: 14
            }
            Text {
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true
                text: info.user.split(' ')[0]
                font.pointSize: 14
            }
        }
        BusyIndicator {
            visible: client.curQuestion === ""
            anchors.centerIn: parent
        }
        Rectangle {
            color: "lightgrey"
            Layout.fillHeight: true;
            Layout.fillWidth: true;
            radius: 10
            visible: client.curQuestion !== ""
            ColumnLayout {
                anchors { fill: parent; margins: 20 }
                RowLayout {
                    Text {
                        Layout.fillHeight: true;
                        Layout.fillWidth: true;
                        Layout.alignment: Qt.AlignTop
                        id: questionText
                        font.pointSize: 12
                        text: client.curQuestion
                        wrapMode: Text.WordWrap
                        textFormat: Text.RichText
                        // Affects fractions
                        //clip: true
                    }
                }
                Repeater {
                    enabled: !(client.curType)
                    id: multichoiceAnswer
                    Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    model: client.curAnswers
                    RowLayout {
                        visible: !(client.curType)
                        Button {
                            text: String.fromCharCode(65+index)
                            onClicked: client.updateDetails(currentQuiz, client.model.getName(currentQuiz), client.model.getPosition(currentQuiz) + 1, String.fromCharCode(65+index))
                            MouseArea {
                                anchors.fill: parent
                                propagateComposedEvents: true

                                onClicked: mouse.accepted = false;
                                onPressed: mouse.accepted = false;
                                onReleased: mouse.accepted = false;
                                onDoubleClicked: mouse.accepted = false;
                                onPositionChanged: mouse.accepted = false;
                                onPressAndHold: mouse.accepted = false;
                                hoverEnabled: true
                                id: buttonMouse
                            }
                        }
                        Text {
                            id: multichoiceText
                            transformOrigin: Item.Left
                            height: contentHeight;
                            smooth: true
                            scale: ((buttonMouse.containsMouse || textMouse.containsMouse) && text.indexOf("<img") == -1) ? 1.5 : 1.0;

                            MouseArea {
                                id: textMouse
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                            Behavior on scale { NumberAnimation { duration: 300;  } }
                            text: modelData
                            textFormat: Text.RichText
                            font.pointSize: 12
                        }
                    }
                }
                RowLayout {
                    id: shortAnswer
                    visible: client.curType
                    Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    spacing: 10
                    Text {
                        visible: text === "$"
                        text: client.curAnswers !== null && client.curAnswers.length ? client.curAnswers[0] : ""
                    }
                    TextField {
                        placeholderText: "Answer"
                        Layout.fillWidth: true
                        focus: visible
                        id: shortAnswerVal
                        Keys.onReturnPressed: submitButton.clicked()
                        Layout.alignment: Qt.AlignLeft
                    }
                    Text {
                        visible: text != "" && text != "$"
                        text: client.curAnswers !== null && client.curAnswers.length ? client.curAnswers[0] : ""
                    }
                    Button  {
                        id: submitButton
                        text: "Submit"
                        Layout.alignment: Qt.AlignRight
                        onClicked: {
                            client.updateDetails(currentQuiz, client.model.getName(currentQuiz), (client.model.getPosition(currentQuiz) + 1), shortAnswerVal.text);
                            shortAnswerVal.text = "";
                        }
                    }
                }
            }
        }

        Button {
            Layout.alignment: Qt.AlignBottom | Qt.AlignRight
            text: "Back"
            onClicked: root.parent.source = ""
        }
    }
}

