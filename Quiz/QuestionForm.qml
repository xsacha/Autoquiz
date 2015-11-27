import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

//import "vars.js" as Vars

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
                Text {
                    Layout.fillHeight: true;
                    Layout.fillWidth: true;
                    Layout.alignment: Qt.AlignTop
                    id: questionText
                    font.pointSize: 12
                    text: client.curQuestion
                    wrapMode: Text.WordWrap
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
                        }
                        Text {
                            text: modelData
                            font.pointSize: 12
                        }
                    }
                }
                RowLayout {
                    id: shortAnswer
                    visible: client.curType
                    Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                    spacing: 10
                    TextField {
                        placeholderText: "Answer"
                        Layout.fillWidth: true
                        focus: visible
                        id: shortAnswerVal
                        Keys.onReturnPressed: submitButton.clicked()
                        Layout.alignment: Qt.AlignLeft
                    }
                    Text {
                        visible: text != ""
                        text: client.curAnswers !== null && client.curAnswers.length ? client.curAnswers[0] : ""
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

