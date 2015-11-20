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
            running: !(client.loggedin)
        }
    }
}

