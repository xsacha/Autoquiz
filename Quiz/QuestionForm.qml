import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2


Item {
    id: root
    width: 640
    height: 480
    property int currentQuiz: (parent === null) ? -1 : parent.currentQuiz;
    RowLayout {
        width: parent.width
        Text {
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            text: client.model.getName(currentQuiz)
            font.pointSize: 14
        }
        Text {
            Layout.alignment: Qt.AlignHCenter  | Qt.AlignTop
            text: "Question " + (client.model.getPosition(currentQuiz) + 1) + "/" + client.model.getTotal(currentQuiz)
            font.pointSize: 14
        }
        Text {
            Layout.alignment: Qt.AlignRight  | Qt.AlignTop
            text: info.user.split(' ')[0]
            font.pointSize: 14
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

