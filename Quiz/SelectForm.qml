import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    id: root
    width: 640
    height: 480
    ListModel {
        //Status
        // 0 : Not started, 1: Partial, 2: Complete
        // 0 allows 'Start', 1 shows partial result and allows 'Resume, 2 shows results.
        id: detailsModel
        // This will eventually come from C++
        ListElement {
            name: "Quiz 1"
            status: 2
            position: 40
            correct: 30
            total: 40
        }
        ListElement {
            name: "Card 1.2.1"
            status: 1
            position: 4
            correct: 0
            total: 10
        }
        ListElement {
            name: "Card 1.2.2"
            status: 0
            position: 0
            correct: 0
            total: 10
        }
        ListElement {
            name: "Card 1.2.3"
            status: 0
            position: 0
            correct: 0
            total: 10
        }
        ListElement {
            name: "Card 1.3.1"
            status: 0
            position: 0
            correct: 0
            total: 10
        }
    }
    Component {
        id: detailsDelegate
        Rectangle {
            NumberAnimation on scale { from: 0.2; to: 1.0; duration: 1000 }
            width: view.cellWidth * 0.9
            height: view.cellHeight * 0.9
            id: rectItem
            MouseArea {
                anchors.fill: parent
                onClicked: view.currentIndex = index
            }
            gradient: Gradient {
                GradientStop { position: 0.0; color: rectItem.GridView.isCurrentItem ? "#ddddddff" : "white" }
                GradientStop { position: 1.0; color: status == 0 ? "lightsteelblue" : (status == 1 ? "orange" : "yellow") }
            }
            border.color: "black"
            border.width: GridView.isCurrentItem ? 2 : 1
            radius: 7
            Item {
                anchors.fill: parent
                Text {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    font.pointSize: 14
                    text: name
                    anchors.top: parent.top
                }
                Text {
                    visible: status == 0
                    text: "Not yet started"
                    font.pointSize: 12
                    anchors.centerIn: parent
                }
                Text {
                    visible: status == 1
                    text: "Partial Complete\n\n(Question " + position + "/" + total + ")"
                    font.pointSize: 12
                    anchors.centerIn: parent
                }
                Text {
                    visible: status == 2
                    text: "Complete\n\n(" + correct + "/" + total + ") Correct"
                    font.pointSize: 12
                    anchors.centerIn: parent
                }
            }
        }
    }
    GridView {
        id: view
        focus: true
        clip: true
        currentIndex: 0
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        width: parent.width - 100
        height: parent.height
        cellWidth: width / 3
        cellHeight: cellWidth
        model: detailsModel
        delegate: detailsDelegate
    }

    ColumnLayout {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        Button {
            property int currentStatus: view.model.get(view.currentIndex).status
            text: currentStatus === 2 ? "Done" : (currentStatus !== 1 ? "Start" : "Resume")
            enabled: currentStatus !== 2
        }
        Button {
            text: "Logout"
            onClicked: info.loggedin = false
        }
    }
}

