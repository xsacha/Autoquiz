import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

ApplicationWindow {
    visible: true
    width: 640
    height: 640
    title: qsTr("Quiz")

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem {
                text: "Exit"
                onTriggered: Qt.quit();
            }
        }
    }

    LoginForm {
        visible: !(client.loggedin)
        anchors.fill: parent
    }
    SelectForm {
        visible: client.loggedin
        anchors.fill: parent
    }

    /*MessageDialog {
        id: messageDialog
        title: qsTr("May I have your attention, please?")

        function show(caption) {
            messageDialog.text = caption;
            messageDialog.open();
        }
    }*/
}

