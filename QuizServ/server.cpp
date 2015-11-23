#include "server.h"
#include <QtNetwork/QNetworkInterface>
#include <QDataStream>
#include <xlsxdocument.h>
//#include <QSqlDatabase>
//#include <QSqlQuery>

Server::Server(QObject *parent) : QTcpServer(parent)
{
    if (!listen(QHostAddress::AnyIPv4, 57849)) {
        qDebug() << "Unable to start the server: " << errorString();

        close();
        return;
    }
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    QFile quizFile("\\\\eqsun2102003\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
    quizFile.open(QIODevice::WriteOnly);
    quizFile.write(QString(ipAddress).toLocal8Bit().toBase64());
    quizFile.close();
    qDebug() << "The server is running.";
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    ServerThread *thread = new ServerThread(socketDescriptor, this);
    qDebug() << "Received connection: " << socketDescriptor;
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

ServerThread::ServerThread(int socketDescriptor, QObject *parent)
    : QThread(parent), socketDescriptor(socketDescriptor)
{
}

void ServerThread::run()
{
    QTcpSocket tcpSocket;
    if (!tcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(tcpSocket.error());
        return;
    }
    tcpSocket.waitForReadyRead(5000);
    QByteArray readBytes = tcpSocket.readAll();
    QDataStream in(&readBytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_4);
    QString username;
    in >> username;

    if (!(username.isEmpty())) {
        qDebug() << "User " << username << " has requested details.";
        readExcelDatabase(username);
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);
        out << (quint16)0;
        out << QString("Quiz 1") << (quint16)2 << (quint16)30 << (quint16)40 << (quint16)40;
        out << QString("Card 1.2.1") << (quint16)1 << (quint16)0 << (quint16)4 << (quint16)10;
        out << QString("Card 1.2.2") << (quint16)0 << (quint16)0 << (quint16)0 << (quint16)10;
        out << QString("Card 1.2.3") << (quint16)0 << (quint16)0 << (quint16)0 << (quint16)10;
        out.device()->seek(0);
        out << (quint16)(block.size() - sizeof(quint16));
        tcpSocket.write(block);
    }
    tcpSocket.disconnectFromHost();
    if (tcpSocket.state() == QAbstractSocket::UnconnectedState ||
            tcpSocket.waitForDisconnected(1000))
        tcpSocket.close();
}

// Using the QtXlsx third-party plugin, we can manipulate Excel 2007+ files from right here.
void ServerThread::readExcelDatabase(QString user) {
    QXlsx::Document xlsx("C:\\Test.xlsx");

    // get the first sheet
    QList<QString> sheetName = xlsx.sheetNames();
    if (!sheetName[0].isEmpty())
    {
        QXlsx::Worksheet *sheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(sheetName[0]));
        if(sheet)
        {
            QXlsx::CellRange range = xlsx.dimension();

            // Cells are 1-based index
            for (int row = 1; row <= range.lastRow(); row++) {
                QString name = xlsx.read(row, 1).toString();
                if (name == user) {
                    // We have a match in the database with the user supplied.
                    // Now we determine their quizzes / scores / positions based on the database.
                    // We probably want to organise a format that is able to include all quizzes,
                    // as the current format only supports a single quiz.
                    qDebug() << xlsx.read(row, 2).toInt();
                }
            }
        }
    }
}
