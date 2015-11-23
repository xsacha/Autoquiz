import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2


Item {
    id: root
    width: 640
    height: 480
    property int currentQuiz: (parent === null) ? -1 : parent.currentQuiz;
    RowLayout {
        Layout.fillWidth: true
    Text {
        Layout.alignment: Qt.AlignLeft
        text: client.model.getName(currentQuiz)
        font.pointSize: 14
    }
    Text {
        Layout.alignment: Qt.AlignCenter
        text: "Question " + (client.model.getPosition(currentQuiz) + 1) + "/" + client.model.getTotal(currentQuiz)
        font.pointSize: 14
    }
    Text {
        Layout.alignment: Qt.AlignRight
        text: ""
    }
    }
    Button {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        text: "Back"
        onClicked: root.parent.source = ""
    }
}

