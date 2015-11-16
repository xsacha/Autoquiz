import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    id: root
    width: 640
    height: 480


    RowLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            property string username: info.user
            text: (busy.running ? "Logging in as " : "") + "<b>" + username + "</b>"
        }
        BusyIndicator {
            id: busy
            running: false
        }
        Button {
            id: loginBtn
            text: "Login"
            onClicked: {
                busy.running = true;
                visible = false;
                // Do work
                loginTimer.start()
            }
        }
        Timer {
            id: loginTimer
            interval: 500
            onTriggered: {
                info.loggedin = true
                busy.running = false;
                loginBtn.visible = true;
            }
        }
    }
}

