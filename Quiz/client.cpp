#include "client.h"
#include <QSettings>
#include <QDataStream>

// Client is started when account info is deemed 'correct' (at start, or when username is changed)
// It immediately begins connection, followed by sending username
// Then we await details.
// Either disconnect or send keep-alives to maintain contact.
// Changing username results in us reconnecting with a new username

Client::Client(QString user, QObject *parent)
    : QTcpSocket(parent), _username(user)
{
    connect(this, SIGNAL(connected()), this, SLOT(sendLogin()));
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(displayError(QAbstractSocket::SocketError)));
    requestDetails();
}

void Client::sendLogin()
{
    _connected = true;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << _username;
    this->write(block);
    connect(this, SIGNAL(readyRead()), this, SLOT(readDetails()));
}

void Client::readDetails()
{
    QDataStream in(this);
    in.setVersion(QDataStream::Qt_5_4);

    if (_blockSize == 0) {
        if (this->bytesAvailable() < (int)sizeof(quint16))
            return;

        in >> _blockSize;
    }

    if (this->bytesAvailable() < _blockSize)
        return;

    QString details;
    in >> details;

    qDebug() << "Received: " << details;
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() << "The host was not found. Please check the host name and port settings.";
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() << "The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct.";
        break;
    default:
        qDebug() << "The following error occurred: " << errorString();
    }
    _connected = false;
}

void Client::requestDetails() {
    _blockSize = 0;
    this->abort();
    this->connectToHost("10.115.80.187", 57849);
}
