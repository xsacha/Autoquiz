import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Quiz")

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem {
                enabled: info.loggedin
                text: "&Logout"
                onTriggered: info.loggedin = false
            }
            MenuItem {
                text: "Exit"
                onTriggered: Qt.quit();
            }
        }
    }

    LoginForm {
        visible: !(info.loggedin)
        anchors.fill: parent
    }
    SelectForm {
        visible: info.loggedin
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

