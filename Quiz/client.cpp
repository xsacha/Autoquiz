#include "client.h"
#include <QSettings>
#include <QDataStream>
#include <QFile>
#include <QTimer>

// Client is started when account info is deemed 'correct' (at start, or when username is changed)
// It immediately begins connection, followed by sending username
// Then we await details.
// Either disconnect or send keep-alives to maintain contact.
// Changing username results in us reconnecting with a new username

Client::Client(QObject *parent)
    : QTcpSocket(parent), _connected(false), _loggedin(false), _sending(false)
{
    accountInfo = new AccountInfo();
    _username = accountInfo->user();
    connect(this, SIGNAL(connected()), this, SLOT(sendData()));
    connect(this, SIGNAL(readyRead()), this, SLOT(readResponse()));
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    _model = new QuizModel();
}

void Client::readResponse()
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

    if (dataPacket.contains("details")) {
        // Populate model with details
    while (!(in.atEnd())) {
        QString quizName;
        qint16 mode, correct, position, total;
        in >> quizName >> mode >> correct >> position >> total;
        _model->addQuiz(QuizInfo(quizName, mode, correct, position, total));
    }
    _loggedin = true;
    loggedinChanged();
    modelChanged();
    } else if (dataPacket.contains("update")) {
        // Check confirm response
        while (!(in.atEnd())) {
            QString confirmText;
            in >> confirmText;
            // If this is "Confirmed" we are OK. Maybe check this?
        }
    }
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() << "The host was not found..";
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() << "The connection was refused by the peer. "
                    "Make sure the server is running.";
        break;
    default:
        qDebug() << "The following error occurred: " << errorString();
    }
    _connected = false;
    _sending = false;
}

void Client::startConnection() {
    _blockSize = 0;
    this->abort();
    // We are grabbing the base64-encoded IP from a common shared drive.
    // T: drive should be visible to all on a typical EQ setup.
    // We use 10.113.28.3 or eqsun2102003 -> Education Queensland | School Code | Drive 3
    QFile quizFile("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
    if (quizFile.exists()) {
        quizFile.open(QIODevice::ReadOnly);
        QString ipAddress = QString(QByteArray::fromBase64(quizFile.readAll()));
        quizFile.close();
        // We are using a hardcoded port
        this->connectToHost(ipAddress, 57849);
        // We are on a LAN so if it doesn't connect in 1.5 seconds, it is never going to connect
        // Re-attempt in 1.5 seconds, which will run this code again if not connected and no model exists.
        QTimer::singleShot(1500, this, SLOT(startConnection()));
    } else {
        // No server turned on. Notify user?
        // Try again in 5 seconds.
        QTimer::singleShot(5000, this, SLOT(startConnection()));
    }
}

void Client::updateDetails() {
    _sending = true;
    if (this->state() == QAbstractSocket::ConnectedState)
        return;
    connect(this, SIGNAL(connected()), this, SLOT(sendData()));
    QDataStream out(&dataPacket, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << "update" << _username;
    startConnection();
}

void Client::requestDetails() {
    _sending = true;
    if (this->state() == QAbstractSocket::ConnectedState)
        return;

    QDataStream out(&dataPacket, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << "details" << _username;
    startConnection();
}

void Client::sendData()
{
    _sending = false;
    _connected = true;
    this->write(dataPacket);
}
