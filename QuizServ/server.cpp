#include "server.h"
#include <QtNetwork/QNetworkInterface>
#include <QDataStream>
#include <xlsxdocument.h>
//#include <QSqlDatabase>
//#include <QSqlQuery>

Server::Server(QObject *parent) : QTcpServer(parent)
{
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
    QFile quizFile("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
    quizFile.open(QIODevice::WriteOnly);
    if (!(quizFile.isOpen()))
    {
        qDebug() << "Unable to create file on network.";
        exit(0);
        return;
    }
    quizFile.write(QString(ipAddress).toLocal8Bit().toBase64());
    quizFile.close();

    if (!listen(QHostAddress::AnyIPv4, 57849)) {
        qDebug() << "Unable to start the server: " << errorString();

        close();
        exit(0);
        return;
    }
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
        QByteArray block = readExcelDatabase(username);
        tcpSocket.write(block);
    }
    tcpSocket.disconnectFromHost();
    if (tcpSocket.state() == QAbstractSocket::UnconnectedState ||
            tcpSocket.waitForDisconnected(1000))
        tcpSocket.close();
}

// Using the QtXlsx third-party plugin, we can manipulate Excel 2007+ files from right here.
QByteArray ServerThread::readExcelDatabase(QString user) {
    // Begin block writing for C++ model
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << (quint16)0;

    QXlsx::Document xlsx("C:\\Test.xlsx");

    QXlsx::Worksheet *summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet("Summary"));
    if(summarySheet)
    {
        QXlsx::CellRange range = summarySheet->dimension();

        // Cells are 1-based index and first index is title
        for (int row = 2; row <= range.lastRow(); row++) {
            QString name = summarySheet->read(row, 1).toString();
            // We have a match in the database with the user supplied.
            // Now we determine their quizzes / scores / positions based on the database.
            // We probably want to organise a format that is able to include all quizzes,
            // as the current format only supports a single quiz.
            if (name == user) {
                // Loop through each quiz
                for (int col = 2; col <= range.lastColumn(); col++) {
                    int total, status, correct, position;
                    // We get the title name from row 1
                    QString quiz = summarySheet->read(1, col).toString();
                    // We get the total for this quiz from the quiz sheet
                    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(QString("%1 Results").arg(quiz)));
                    if (quizSheet == nullptr)
                        break;
                    // It is a formula so we need to find the value()
                    total = quizSheet->cellAt(2,1)->value().toInt();
                    // The summary tells us the status and the correct/position
                    QStringList summaryDetail = summarySheet->read(row, col).toString().split(',');
                    status = summaryDetail.first().toInt();
                    if (status == 2) {
                        correct = summaryDetail.last().toInt();
                        position = total; // We are at end.
                    }
                    else if (status == 1) {
                        position = summaryDetail.last().toInt();
                        correct = 0; // Assume we haven't counted yet. Maybe we will in future?
                    } else {
                        position = 0; // We haven't started.
                        correct = 0;
                    }
                    out << quiz << (quint16)status << (quint16)correct << (quint16)position << (quint16)total;
                }
            }
        }
    }
    // Complete block and return
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    return block;
}
