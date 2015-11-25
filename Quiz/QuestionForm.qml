import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

import "vars.js" as Vars

Item {
    id: root
    width: 640
    height: 480
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
                Layout.fillHeight: true;
                Layout.fillWidth: true;
                Text {
                    Layout.alignment: Qt.AlignTop
                    id: questionText
                    font.pointSize: 12
                    text: { var str = client.curQuestion;
                        str.replace(/{Name}/g, Vars.randName());
                    }
                }
                Repeater {
                    id: multichoiceAnswer
                    visible: client.curType === 0
                    Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    model: client.curAnswers
                    RowLayout {
                        Button {
                            text: String.fromCharCode(65+index)
                            onClicked: client.updateDetails(currentQuiz, client.model.getName(currentQuiz), client.model.getPosition(currentQuiz) + 1, String.fromCharCode(65+index))
                        }
                        Text {
                            text: modelData
                            font.pointSize: 12
                        }
                    }
                }
                RowLayout {
                    id: shortAnswer
                    visible: client.curType === 1
                    Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    TextEdit {
                        id: shortAnswerVal
                        Keys.onReturnPressed: submitButton.clicked()
                    }
                    Button  {
                        id: submitButton
                        text: "Submit"
                        Layout.alignment: Qt.AlignRight
                        onClicked: client.updateDetails(currentQuiz, client.model.getName(currentQuiz), (client.model.getPosition(currentQuiz) + 1), shortAnswerVal.text);
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

