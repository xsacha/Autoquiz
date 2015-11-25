#include "server.h"
#include <QtNetwork/QNetworkInterface>
#include <QDataStream>
#include <xlsxdocument.h>
//#include <QSqlDatabase>
//#include <QSqlQuery>

#ifdef WINVER
#define XLSX_FILE "C:\\Test.xlsx"
#else
#define XLSX_FILE "Test.xlsx"
#endif

Server::Server(QObject *parent) : QTcpServer(parent)
{
    quint32 ipAddress = 0;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toIPv4Address();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress == 0)
        ipAddress = QHostAddress(QHostAddress::LocalHost).toIPv4Address();
    QFile quizFile("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
    quizFile.open(QIODevice::WriteOnly);
    if (!(quizFile.isOpen()))
    {
        qDebug() << "Unable to create file on network.";
        exit(0);
        return;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly | QIODevice::Truncate);
    out.setVersion(QDataStream::Qt_5_4);
    out << ipAddress;
    quizFile.write(block);
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
    tcpSocket.waitForReadyRead(2000);
    QByteArray readBytes = tcpSocket.readAll();
    QDataStream in(&readBytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_4);
    QString command, username;
    in >> command >> username;

    QByteArray block;
    if (command == "details") {
        if (!(username.isEmpty())) {
            qDebug() << "User " << username << " has connected and requested details.";
            block = readExcelDatabase(username);
        }
    } else if (command == "question") {
        QString quizName;
        quint16 question;
        in >> quizName >> question;
        block = sendUserQuestion(quizName, question);
    } else if (command == "update" || command == "updatelast") {
        if (!(username.isEmpty())) {
            qDebug() << "User " << username << " has updated database.";
            QString quizName;
            quint16 question;
            QString answer;
            in >> quizName >> question >> answer;
            if (answer.isEmpty())
                answer = "-";
            // Blank answer means question wasn't done. Since it was done, we'll make this a line.
            block = updateUserAnswer(username, quizName, question, answer);
        }
    } else {
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);
        out << (quint16)0 << QString("Error");
    }
    // Write first byte as size and send block
    QDataStream out (&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    tcpSocket.write(block);

    // Now we will disconnect from client.
    tcpSocket.disconnectFromHost();
    if (tcpSocket.state() == QAbstractSocket::UnconnectedState ||
            tcpSocket.waitForDisconnected(500))
        tcpSocket.close();
}

// Using the QtXlsx third-party plugin, we can manipulate Excel 2007+ files from right here.
QByteArray ServerThread::readExcelDatabase(QString user) {
    // Begin block writing for C++ model
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << (quint16)0;

    QXlsx::Document xlsx(XLSX_FILE);

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
                break;
            }
            if (row == range.lastRow()) {
                // Add new user
                ++row;
                summarySheet->writeString(row, 1, user);
                // We are just initialising a new user with all zeros
                for (int col = 2; col <= range.lastColumn(); col++) {
                    summarySheet->writeString(row, col, "0,0");
                    QString quiz = summarySheet->read(1, col).toString();

                    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(QString("%1 Results").arg(quiz)));
                    out << quiz << (quint16)0 << (quint16)0 << (quint16)0 << (quint16)quizSheet->cellAt(2,1)->value().toInt();;
                }
                // Save the file at conclusion of changes
                xlsx.save();
                break;
            }
        }

    }

    return block;
}

QByteArray ServerThread::updateUserAnswer(QString username, QString quizName, int question, QString answer) {
    QXlsx::Document xlsx(XLSX_FILE);

    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(QString("%1 Results").arg(quizName)));

    QByteArray block;
    int lastRow = quizSheet->dimension().lastRow();
    for (int row = 6; row <= lastRow; row++) {
        if (quizSheet->read(row, 1).toString() == username) {
            quizSheet->writeString(row, 1 + question, answer);
            break;
        }
        // User hasn't started this quiz before, so try to add their name!
        if (row == lastRow) {
            ++row;
            quizSheet->writeString(row, 1, username);
            quizSheet->writeString(row, 1 + question, answer);
            break;
        }
    }
    // We need to know total to work out if we finished this quiz.
    int total = quizSheet->cellAt(2,1)->value().toInt();
    QXlsx::Worksheet *summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet("Summary"));
    int lastColumn = summarySheet->dimension().lastColumn();
    lastRow = summarySheet->dimension().lastRow();
    for (int col = 2; col <= lastColumn; col++) {
        if (summarySheet->read(1, col).toString() == quizName) {
            for (int row = 2; row <= lastRow; row++) {
                if (summarySheet->read(row, 1).toString() == username) {
                    summarySheet->writeString(row, col, QString("%1,%2").arg(question == total ? "2" : "1").arg(QString::number(question)));
                    break;
                }
                // Will already be in this sheet from when logged in. No need to check if they exist
            }
            break;
        }
    }
    // Save the file
    xlsx.save();
    if (question != total) {
        block = sendUserQuestion(quizName, question + 1);
    } else {
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);
        out << (quint16)0 << QString("End");
    }
    return block;
}

QByteArray ServerThread::sendUserQuestion(QString quizName, int question)
{
    // Begin block writing for C++ model
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << (quint16)0;
    // Type: MC (0) or SA (1)
    out << (bool)0;
    // Question
    out << QString("{Name} made some cookie men.\n"
           "Each cookie man had 2 star lollies and 3 round lollies.\n"
           "{Name} used 48 round lollies altogether.\n\n"
           "How many star lollies did {Name} use?");
    // Answer
    out << (QStringList() << "16" << "24" << "32" << "48" << "96");
    return block;
}
