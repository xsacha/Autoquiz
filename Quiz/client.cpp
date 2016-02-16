#include "client.h"
#include <QSettings>
#include <QDataStream>
#include <QFile>
#include <QTimer>
#include <QImage>
#include <QHostAddress>
#include <QRegularExpression>

// Should this be configurable like server?
#ifdef WINVER
#define QUIZPATH QString("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\")
#else
#define QUIZPATH QString("/data/build/")
#endif

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

    QString command;
    QDataStream readPacket(dataPacket);
    readPacket.setVersion(QDataStream::Qt_5_4);
    readPacket >> command;
    if (command == "details") {
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
    } else if (command == "question" || command == "update") {
        // Receive question / answers / images
        in >> _curType >> _curQuestion >> _curAnswers;
        QStringList fname = QStringList() << "Amanda" << "Samantha" << "Laura" << "Lorna" << "Alana" << "Nicole" << "Sandra" << "Tayla" << "Tia" << "Jessica" << "Yvonne" << "Michelle" << "Jane" << "Irene";
        QStringList mname = QStringList() << "Adam" << "Joal" << "Phil" << "James" << "Simon" << "Nathan" << "William" << "Sacha" << "Jamie" << "Jayden" << "Kyle" << "Paul" << "Gregory" << "Peter";
        _curQuestion.replace("{FName}", fname.at(rand() % fname.length()), Qt::CaseInsensitive);
        _curQuestion.replace("{MName}", mname.at(rand() % mname.length()), Qt::CaseInsensitive);
        QRegularExpression reimg("{Img_(.*)}");
        reimg.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = reimg.match(_curQuestion);
        if (m.hasMatch()) {
            foreach(QString captureText, m.capturedTexts()) {
                QString tempString = captureText;
                tempString.replace("{Img_","<br><table align=\"center\"><tr><td><img src=\"file://10.113.28.3/Data/Curriculum/Common/Maths/", Qt::CaseInsensitive);
                tempString.replace("}", "\"></td></tr></table><br>");
                _curQuestion.replace(captureText, tempString);
            }
        }
        QRegularExpression refrac("{Frac_(.*)}");
        refrac.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        m = refrac.match(_curQuestion);
        if (m.hasMatch()) {
            foreach(QString captureText, m.capturedTexts()) {
                if (!(captureText.contains('/')))
                    continue;
                QString tempString = captureText;
                tempString.replace("{Frac_","<sup>", Qt::CaseInsensitive);
                tempString.replace("/", "</sup>&frasl;<sub>");
                tempString.replace("}", "</sub>");
                _curQuestion.replace(captureText, tempString);
            }
        }
        for (int i = 0; i < _curAnswers.count(); i++) {
            m = reimg.match(_curAnswers[i]);
            if (m.hasMatch()) {
                foreach(QString captureText, m.capturedTexts()) {
                    QString tempString = captureText;
                    tempString.replace("{Img_","<img src=\"file://10.113.28.3/Data/Curriculum/Common/Maths/", Qt::CaseInsensitive);
                    tempString.replace("}", "\">");
                    _curAnswers[i].replace(captureText, tempString);
                }
            }
            m = refrac.match(_curAnswers[i]);
            if (m.hasMatch()) {
                foreach(QString captureText, m.capturedTexts()) {
                    if (!(captureText.contains('/')))
                        continue;
                    QString tempString = captureText;
                    tempString.replace("{Frac_","<sup>", Qt::CaseInsensitive);
                    tempString.replace("/", "</sup>&frasl;<sub>");
                    tempString.replace("}", "</sub>");
                    _curAnswers[i].replace(captureText, tempString);
                }
            }
        }
        questionChanged();
        modelChanged(); // Updates Position and mode
    } else if (command == "updatelast") {
        // Check correct response
        quint16 currentQuiz, correct;
        in >> currentQuiz >> correct;
        _model->setCorrect(currentQuiz, correct);
        modelChanged(); // Updates UI for completed quiz
    }
}
void Client::displayError(QAbstractSocket::SocketError socketError)
{
    _connected = false;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() << "The host was not found..";
        // Server not running yet, try again in 3000 ms
        QTimer::singleShot(3000, this, SLOT(startConnection()));
        return;
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() << "The connection was refused by the peer. "
                    "Make sure the server is running.";
        // Server not running yet, try again in 3000 ms
        QTimer::singleShot(3000, this, SLOT(startConnection()));
        return;
        break;
    default:
        qDebug() << "The following error occurred: " << errorString();
    }
    _sending = false;
}

void Client::startConnection() {
    if (!_sending)
        return;
    _blockSize = 0;
    this->abort();
    // We are grabbing the IP from a common shared drive.
    // T: drive should be visible to all on a typical EQ setup.
    // We use 10.113.28.3 or eqsun2102003 -> Education Queensland | School Code | Drive 3
    QFile quizFile(QUIZPATH+"Quiz.txt");
    if (quizFile.exists()) {
        quizFile.open(QIODevice::ReadOnly);
        QByteArray block = quizFile.readAll();
        QDataStream in(&block, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_4);
        quint32 ip;
        quint16 port;
        in >> ip >> port;
        QHostAddress ipHost;
        ipHost.setAddress(ip);
        quizFile.close();
        this->connectToHost(ipHost, port);
        // We are on a LAN so if it doesn't connect in 1.5 seconds, it is never going to connect
        // Re-attempt in 1.5 seconds, which will run this code again if not connected and no model exists.
        QTimer::singleShot(1500, this, SLOT(startConnection()));
    } else {
        // No server turned on. Notify user?
        // Try again in 5 seconds.
        QTimer::singleShot(5000, this, SLOT(startConnection()));
    }
}

void Client::updateDetails(int currentQuiz, QString quizName, quint16 position, QString value) {
    _sending = true;
    if (this->state() == QAbstractSocket::ConnectedState)
        return;

    dataPacket = QByteArray();
    QDataStream out(&dataPacket, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    _model->setPosition(currentQuiz, position);
    if (_model->getPosition(currentQuiz) == _model->getTotal(currentQuiz))
        _model->setMode(currentQuiz, 2);
    out << ((_model->getMode(currentQuiz) == 2) ? QString("updatelast") : QString("update"))
        << _username << (quint16)currentQuiz << quizName << position << value;
    _curQuestion = "";
    questionChanged();
    startConnection();
}

void Client::requestDetails() {
    _sending = true;
    if (this->state() == QAbstractSocket::ConnectedState)
        return;

    dataPacket = QByteArray();
    QDataStream out(&dataPacket, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << QString("details") << _username;
    startConnection();
}

void Client::requestQuestion(QString quizName, quint16 question) {
    _sending = true;
    if (this->state() == QAbstractSocket::ConnectedState)
        return;

    dataPacket = QByteArray();
    QDataStream out(&dataPacket, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << QString("question") << _username << quizName << question;
    startConnection();
}

void Client::sendData()
{
    _sending = false;
    _connected = true;
    this->write(dataPacket);
}
