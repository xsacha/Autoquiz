#include "client.h"
#include <QSettings>
#include <QDataStream>
#include <QFile>

// Client is started when account info is deemed 'correct' (at start, or when username is changed)
// It immediately begins connection, followed by sending username
// Then we await details.
// Either disconnect or send keep-alives to maintain contact.
// Changing username results in us reconnecting with a new username

Client::Client(QObject *parent)
    : QTcpSocket(parent)
{
    accountInfo = new AccountInfo();
    _username = accountInfo->user();
    connect(this, SIGNAL(connected()), this, SLOT(sendLogin()));
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(displayError(QAbstractSocket::SocketError)));
    requestDetails();
    _model = new QuizModel();
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

    _model->addQuiz(QuizInfo("Quiz 1", 2, 30, 40, 40));
    modelChanged();
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
}

void Client::requestDetails() {
    _blockSize = 0;
    this->abort();
    // We are grabbing the base64-encoded IP from a common shared drive.
    // T: drive should be visible to all on a typical EQ setup.
    QFile quizFile("\\\\eqsun2102003\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
    quizFile.open(QIODevice::ReadOnly);
    QString ipAddress = QString(QByteArray::fromBase64(quizFile.readAll()));
    quizFile.close();
    // We are using a hardcoded port
    this->connectToHost(ipAddress, 57849);
}
