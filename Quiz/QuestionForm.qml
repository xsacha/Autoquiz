import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2


Item {
    id: root
    width: 640
    height: 480
    property int currentQuiz: (parent === null) ? -1 : parent.currentQuiz;
    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        text: client.model.getName(currentQuiz)
    }
    Text {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Question " + (client.model.getPosition(currentQuiz) + 1) + "/" + client.model.getTotal(currentQuiz)
    }
    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        text: ""
    }
    Button {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        text: "Back"
        onClicked: root.parent.source = ""
    }
}

