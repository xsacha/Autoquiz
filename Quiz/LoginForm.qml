import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2

Item {
    id: root
    width: 640
    height: 640

    RowLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            property string username: info.user
            text: (busy.running ? "Logging in as " : "") + "<b>" + username + "</b>"
        }
        // Create a short delay so we can start UI before networking.
        Timer {
            interval: 100
            running: true
            onTriggered: client.requestDetails()
        }

        BusyIndicator {
            id: busy
            running: !(client.loggedin)
        }
    }
}

