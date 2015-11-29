#include "server.h"
#include <QtNetwork/QNetworkInterface>
#include <QDataStream>
#include <xlsxcellformula.h>

Server::Server(QObject *parent)
    : QTcpServer(parent)
    #ifdef WINVER
    , ipDiscoveryPath("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\")
    , xlsxPath("\\\\10.113.28.1\\Data\\CoreData\\Common\\Maths\\Quiz\\")
    #else
    , ipDiscoveryPath("/data/build/")
    , xlsxPath("/data/build/")
    #endif
{
    // Start server
    if (!listen(QHostAddress::AnyIPv4, 57849)) {
        qDebug() << "Unable to start the server: " << errorString();

        close();
        exit(0);
        return;
    }
    qDebug() << "The server is running.";

    // Check if customised paths exist
    QFile loadPaths("Paths.txt");
    loadPaths.open(QIODevice::ReadOnly);
    if (loadPaths.isOpen()) {
        while (!loadPaths.atEnd()) {
            QString line = QString(loadPaths.readLine()).split("//").first();
            QStringList parts = line.split(":");
            QString pathType = parts.first().toLower();
            QString pathResult = parts.at(1).simplified();
            if (parts.length() < 2)
                continue;
            if (pathType == "ip discovery") {
                ipDiscoveryPath = pathResult;
            } else if (pathType == "spreadsheets") {
                xlsxPath = pathResult;
            }
        }
        loadPaths.close();
    }

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
    quizFile = new QFile(ipDiscoveryPath+"Quiz.txt");
    quizFile->open(QIODevice::WriteOnly);
    if (!(quizFile->isOpen()))
    {
        qDebug() << "Unable to create file on network.";
        exit(0);
        return;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly | QIODevice::Truncate);
    out.setVersion(QDataStream::Qt_5_4);
    out << ipAddress;
    quizFile->write(block);
    quizFile->close();

    // Check questions files for any updates. Update results accordingly.
    QXlsx::Document xlsx(xlsxPath+"Results.xlsx");
    QXlsx::Worksheet *summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet("Summary"));
    QXlsx::Document xlsxq(xlsxPath+"Questions.xlsx");
    QXlsx::Format boldFormat;
    boldFormat.setFontBold(true);
    foreach(QString name, xlsxq.sheetNames()) {

        for (int i = 1; i <= summarySheet->dimension().columnCount(); i++) {
            if (i != 1 && summarySheet->read(1, i).toString() == name) {
                // Found it
                QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(name));
                if (quizSheet != nullptr) {
                    // It is possible that quizSheet doesn't exist. In this case we want to leave it alone for now.
                    // It may not be ready yet so when it is ready and has been used, the sheet will generate.
                    int total = quizSheet->cellAt(1, 1)->value().toInt();
                    QXlsx::Worksheet *questionSheet = dynamic_cast<QXlsx::Worksheet *>(xlsxq.sheet(name));
                    int newTotal = questionSheet->dimension().rowCount();
                    // Verify we have as many questions in this sheet as we did last time we loaded it
                    if (total != newTotal) {
                        if (total < newTotal) {
                            qDebug() << "The sheet " << name << " has less questions than it did before. This may cause issues.\nPlease fix this. Before: " << total << " Now: " << newTotal;
                        } else {
                            qDebug() << "The sheet " << name << " has more questions than it did before. Extending answer sheet.";
                            // Move column 2 + total to 2 + newTotal
                            for (int r = 1; r <= quizSheet->dimension().rowCount(); r++) {
                                quizSheet->write(r, 2 + newTotal, quizSheet->read(r, 2 + total), boldFormat);
                                quizSheet->writeBlank(r, 2 + total);
                            }
                            // Fill in gaps
                            for (int c = total + 1; c <= newTotal + 1; c++) {
                                quizSheet->writeString(2, c, QString::number(c-1), boldFormat);
                            }
                            // Just replace all answers, just in case!
                            for (int i = 1; i <= newTotal; i++) {
                                quizSheet->writeString(3, 1 + i, questionSheet->cellAt(1 + i, 3)->value().toString());
                            }
                            // We need code to update what happens to the sheets once the questions have changed.
                            // We can assume no real students have taken the test at this stage.
                            // Probably make every student back to status 1 (if they were 2).
                            xlsx.save();
                        }
                    }
                }
                // On to next name
                break;
            }
            if (i == summarySheet->dimension().columnCount()) {
                // We don't have this quiz here yet. Add it!
                ++i;
                summarySheet->writeString(1, i, name, boldFormat);
                for (int j = 2; j <= summarySheet->dimension().rowCount(); j++) {
                    // For each student, add the quiz score as status 0, position 0
                    summarySheet->write(j, i, "0,0");
                }
                xlsx.save();
            }
        }
    }
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    ServerThread *thread = new ServerThread(socketDescriptor, xlsxPath, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

ServerThread::ServerThread(int socketDescriptor, QString path, QObject *parent)
    : QThread(parent), socketDescriptor(socketDescriptor), xlsxPath(path)
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
            quint16 currentQuiz;
            // Client will want to know what quiz to update 'correct' for, in model
            in >> currentQuiz;
            QString quizName;
            quint16 question;
            QString answer;
            in >> quizName >> question >> answer;
            // Blank answer means question wasn't done. Since it was done, we'll make this a line.
            if (answer.isEmpty())
                answer = "-";

            block = updateUserAnswer(username, quizName, question, answer);
            // This is the last question in the quiz.
            // We will give them how many were correct instead of next question.
            if (command == "updatelast") {
                quint16 correct = 0;
                QXlsx::Document xlsx(xlsxPath+"Results.xlsx");

                QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));
                int lastRow = quizSheet->dimension().lastRow();
                int lastCol = quizSheet->dimension().lastColumn();
                for (int row = 5; row <= lastRow; row++) {
                    // Found the user
                    if (quizSheet->read(row, 1).toString() == username) {
                        // Their total score for this quiz is in the last column
                        correct = quizSheet->cellAt(row, lastCol)->value().toInt();
                        break;
                    }
                }
                QDataStream out(&block, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_5_4);
                out << (quint16)0 << currentQuiz << correct;
            }
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

    QXlsx::Document xlsx(xlsxPath+"Results.xlsx");

    QXlsx::Worksheet *summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet("Summary"));
    if(summarySheet == nullptr)
    {
        createSummarySheet(&xlsx);
        summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet("Summary"));
    }
    QXlsx::CellRange range = summarySheet->dimension();

    // Cells are 1-based index and first index is title
    for (int row = 1; row <= range.lastRow(); row++) {
        QString name = summarySheet->read(row, 1).toString();
        // We have a match in the database with the user supplied.
        // Now we determine their quizzes / scores / positions based on the database.
        if (row != 1 && name == user) {
            // Loop through each quiz
            for (int col = 2; col <= range.lastColumn(); col++) {
                int total, status, correct, position;
                // We get the title name from row 1
                QString quiz = summarySheet->read(1, col).toString();
                // We get the total for this quiz from the quiz sheet

                QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quiz));
                if (quizSheet == nullptr) {
                    createQuizSheet(&xlsx, quiz);
                    quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quiz));
                }
                // It is a formula so we need to find the value()
                total = quizSheet->cellAt(1,1)->value().toInt();
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

                QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quiz));
                if (quizSheet == nullptr) {
                    createQuizSheet(&xlsx, quiz);
                    quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quiz));
                }
                out << quiz << (quint16)0 << (quint16)0 << (quint16)0 << (quint16)quizSheet->cellAt(1,1)->value().toInt();
            }
            // Save the file at conclusion of changes
            xlsx.save();
            break;
        }

    }

    return block;
}

void ServerThread::createSummarySheet(QXlsx::Document * xlsx) {
    QXlsx::Document xlsxq(xlsxPath+"Questions.xlsx");
    xlsx->addSheet("Summary");
    QXlsx::Worksheet *summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx->sheet("Summary"));
    int i = 0;
    QXlsx::Format boldFormat;
    boldFormat.setFontBold(true);
    summarySheet->write(1, 1, "Student", boldFormat);
    foreach(QString name, xlsxq.sheetNames()) {
        summarySheet->write(1, 2 + i, name, boldFormat);
    }
    xlsx->save();
}

void ServerThread::createQuizSheet(QXlsx::Document *xlsx, QString quizName) {
    QXlsx::Document xlsxq(xlsxPath+"Questions.xlsx");
    xlsx->addSheet(quizName);
    QXlsx::Worksheet *questionSheet = dynamic_cast<QXlsx::Worksheet *>(xlsxq.sheet(quizName));
    int total = questionSheet->dimension().rowCount() - 1;
    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx->sheet(quizName));
    quizSheet->writeString(1, 1, QString::number(total));
    quizSheet->writeFormula(1, 2, QXlsx::CellFormula("=COUNTA(A:A)-4"));
    QXlsx::Format boldFormat;
    boldFormat.setFontBold(true);
    // Count Row
    quizSheet->writeString(2, 1, "Students", boldFormat);
    for (int i = 1; i <= total; i++) {
        quizSheet->writeString(2, 1 + i, QString::number(i), boldFormat);
    }
    quizSheet->writeString(2, 2 + total, "Total", boldFormat);
    quizSheet->setColumnWidth(2, 1 + total, 2.0); // Set widths for counts/answers
    // Answer Row
    quizSheet->writeString(3, 1, "Answer", boldFormat);
    for (int i = 1; i <= total; i++) {
        quizSheet->writeString(3, 1 + i, questionSheet->cellAt(1 + i, 3)->value().toString());
    }
    // Total Row
    quizSheet->writeString(4, 1, "Total", boldFormat);
    for (int i = 1; i <= total; i++) {
        quizSheet->writeFormula(4, 1 + i, QXlsx::CellFormula("=COUNTIF(" + QXlsx::CellReference(5, 1 + i).toString(true, false) + ":OFFSET(" + QXlsx::CellReference(5, 1 + i).toString(true, false) + ",$B$1,0)," + QXlsx::CellReference(3, 1 + i).toString(true, false) + ")"), boldFormat);
    }
    quizSheet->writeFormula(4, 2 + total, QXlsx::CellFormula("=SUM(B4:" + QXlsx::CellReference(2, 1 + total).toString() + ")"), boldFormat);
    xlsx->save();
}

QByteArray ServerThread::updateUserAnswer(QString username, QString quizName, int question, QString answer) {
    QXlsx::Document xlsx(xlsxPath+"Results.xlsx");

    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));

    QByteArray block;
    if (quizSheet == nullptr) {

        createQuizSheet(&xlsx, quizName);
        quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));
    }

    // We need to know total to work out if we finished this quiz.
    int total = quizSheet->cellAt(1,1)->value().toInt();

    QXlsx::CellRange range = quizSheet->dimension();
    for (int row = 5; row <= range.lastRow() + 1; row++) {
        if (quizSheet->read(row, 1).toString() == username) {
            quizSheet->writeString(row, 1 + question, answer);
            break;
        }
        // User hasn't started this quiz before, so try to add their name!
        if (row >= range.lastRow()) {
            ++row;
            quizSheet->writeString(row, 1, username);
            quizSheet->writeString(row, 1 + question, answer);
            quizSheet->writeFormula(row, 2 + total, QXlsx::CellFormula("=SUMPRODUCT(--(" + QXlsx::CellReference(row, 2).toString(false, true) + ":" + QXlsx::CellReference(row, 1+total).toString(false, true) + "=$B$3:" + QXlsx::CellReference(3, 1+total).toString(true, true) +  "))"));
            break;
        }
    }

    QXlsx::Worksheet *summarySheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet("Summary"));
    range = summarySheet->dimension();
    for (int col = 2; col <= range.lastColumn(); col++) {
        if (summarySheet->read(1, col).toString() == quizName) {
            for (int row = 2; row <= range.lastRow(); row++) {
                if (summarySheet->read(row, 1).toString() == username) {
                    summarySheet->writeString(row, col, QString("%1,%2").arg((question == total) ? "2" : "1").arg(QString::number(question)));
                    break;
                }
                // Will already be in this sheet from when logged in. No need to check if they exist
            }
            break;
        }
    }
    // Save the file
    xlsx.save();
    if (question < total) {
        block = sendUserQuestion(quizName, question + 1);
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
    QXlsx::Document xlsx(xlsxPath+"Questions.xlsx");
    QXlsx::Worksheet *questionSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));
    bool sa = true;
    QString questionStr = questionSheet->cellAt(1 + question, 1)->value().toString();
    QString answers = questionSheet->cellAt(1 + question, 2)->value().toString();
    if (answers.contains(','))
        sa = false;
    // Type: MC (0) or SA (1)
    // Question and then Answers list
    out << sa << questionStr << answers.split(',');
    return block;
}
