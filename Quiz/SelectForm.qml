import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    id: root
    width: 640
    height: 480
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
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        width: parent.width - 100
        height: parent.height
        cellWidth: width / 3
        cellHeight: cellWidth
        model: client.model
        delegate: detailsDelegate
    }

    ColumnLayout {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        Button {
            property int currentStatus: view.model.getStatus(view.currentIndex)
            text: currentStatus === 2 ? "Done" : (currentStatus === 1 ? "Resume" : "Start")
            enabled: currentStatus !== 2
        }
        /*Button {
            text: "Logout"
            onClicked: info.loggedin = false
        }*/
    }
}

