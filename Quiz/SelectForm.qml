import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    id: root
    width: 640
    height: 640
    Loader {
        id: question
        property int currentQuiz: view.currentIndex
        anchors.fill: parent
    }

    Component {
        id: detailsDelegate
        Rectangle {
            width: view.cellWidth * 0.9
            height: view.cellHeight * 0.9
            id: rectItem
            MouseArea {
                anchors.fill: parent
                onClicked: view.currentIndex = index
                onDoubleClicked: { view.currentIndex = index; if (goButton.enabled) goButton.clicked(); }
            }
            gradient: Gradient {
                GradientStop { position: 0.0; color: rectItem.GridView.isCurrentItem ? "#ddddddff" : "white" }
                GradientStop { position: 1.0; color: mode == 0 ? "lightsteelblue" : (mode == 1 ? "orange" : "yellow") }
            }
            border.color: "black"
            border.width: GridView.isCurrentItem ? 2 : 1
            radius: 7
            ColumnLayout {
                anchors.fill: parent
                Text {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    font.pointSize: 14
                    text: name
                    anchors.top: parent.top
                }
                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Image {
                        fillMode: Image.PreserveAspectFit
                        anchors.fill: parent
                        source: (mode == 2 ? "complete.png" : (mode == 1 ? "suspend.png" : "play.png"))
                        opacity: 0.2
                    }

                    Text {
                        visible: mode == 0
                        text: "Not yet started"
                        font.pointSize: 12
                        anchors.centerIn: parent
                    }
                    Text {
                        visible: mode == 1
                        property int nextQ: position + 1
                        text: "Partial Complete\n\n(Question " + nextQ + "/" + total + ")"
                        font.pointSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        anchors.centerIn: parent
                    }
                    Text {
                        visible: mode == 2
                        text: "Complete\n\n(" + correct + "/" + total + " Correct)"
                        font.pointSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        anchors.centerIn: parent
                    }

                }
            }
        }
    }

    ColumnLayout {
        visible: question.source == ""
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            horizontalAlignment: Text.AlignHCenter
            text: "Welcome, " + info.user.split(' ')[0]
            font.pointSize: 14
        }
        GridView {
            id: view
            Layout.fillHeight: true
            Layout.fillWidth: true
            populate: Transition {
                NumberAnimation {
                    property: "scale"
                    from: 0
                    duration: 500
                }
            }
            focus: true
            clip: true
            currentIndex: -1
            width: Layout.width
            height: Layout.height
            cellWidth: width / 3
            cellHeight: cellWidth * 2 / 3
            onVisibleChanged: if (visible) { model = ListModel; model = client.model }
            delegate: detailsDelegate
        }
        Button {
            id: goButton
            Layout.alignment: Qt.AlignCenter
            property string currentName: client.model.getName(view.currentIndex)
            property int currentMode: client.model.getMode(view.currentIndex)
            property int currentPos: client.model.getPosition(view.currentIndex)
            enabled: currentMode != 2 && currentPos > -1 && question.source == ""
            onCurrentModeChanged: if (currentMode == 2 && question.source !== "") { question.source = ""; }
            text: enabled ? (currentMode == 1 ? "Resume" : "Start") : "Done";
            onClicked: { question.source = "QuestionForm.qml"; client.requestQuestion(currentName, currentPos + 1); }
        }
    }
}
