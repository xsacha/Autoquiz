import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    id: root
    width: 640
    height: 480
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
                        text: "Partial Complete\n\n(Question " + position + "/" + total + ")"
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
                    property: "x"
                    from: -100
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
            model: client.model
            delegate: detailsDelegate
        }
        Button {
            Layout.alignment: Qt.AlignCenter
            property int currentmode: view.model.getMode(view.currentIndex)
            text: currentmode === 2 ? "Done" : (currentmode === 1 ? "Resume" : "Start")
            enabled: currentmode !== 2
            onClicked: question.source = "QuestionForm.qml"
        }
    }
}
