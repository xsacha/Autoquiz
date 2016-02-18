#include "server.h"
#include <QtNetwork/QNetworkInterface>
#include <QDataStream>
#include <xlsxcellformula.h>
#include <QDir>
#include <QRegularExpression>
#include <QImage>

Server::Server(QObject *parent)
    : QTcpServer(parent)
    #ifdef WINVER
    , ipDiscoveryPath("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\Quiz\\")
    , xlsxPath("\\\\10.113.28.1\\Data\\CoreData\\Common\\Maths\\Quiz\\")
    #else
    , ipDiscoveryPath("/data/build/")
    , xlsxPath("/data/build/")
    #endif
{
    // Start server
    if (!listen(QHostAddress::AnyIPv4)) {
        qDebug() << "Unable to start the server: " << errorString();

        close();
        exit(0);
        return;
    }

    // Check if customised paths exist
    QFile loadPaths("Paths.txt");
    loadPaths.open(QIODevice::ReadOnly);
    if (loadPaths.isOpen()) {
        while (!loadPaths.atEnd()) {
            QString line = QString(loadPaths.readLine()).split("//").first();
            QStringList parts = line.split(": ");
            QString pathType = parts.first().toLower();
            QString pathResult = parts.at(1).simplified();
            if (parts.length() < 2)
                continue;
            if (pathType == "ip discovery") {
                ipDiscoveryPath = pathResult;
                // Update old path. Remove this later.
                if (ipDiscoveryPath == "\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\") {
                    ipDiscoveryPath = "\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\Quiz\\";
                }
            } else if (pathType == "spreadsheets") {
                xlsxPath = pathResult;
            }
        }
        loadPaths.close();
    }
    QDir().mkpath(ipDiscoveryPath);


    quint32 ipAddress = 0;
    QString ipString;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first 10.* IPv4 address. Probably bad for portability but works at PRSHS
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).toString().startsWith("10") &&
                ipAddressesList.at(i).toIPv4Address()) {
            ipString = ipAddressesList.at(i).toString();
            ipAddress = ipAddressesList.at(i).toIPv4Address();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress == 0)
        ipAddress = QHostAddress(QHostAddress::LocalHost).toIPv4Address();

    QFile testFile(ipDiscoveryPath+"Quiz.txt");
    if (testFile.exists()) {
        testFile.open(QIODevice::ReadOnly);
        QByteArray block = testFile.readAll();
        QDataStream in(&block, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_4);
        quint32 ip;
        in >> ip;
        if (ip == ipAddress) {
            qDebug() << "Cleaning up last run on this machine.";
        } else {
            qDebug() << "Warning: Server was already running on another machine! Shutting down remote server.";
        }
        testFile.remove();
    }
    qDebug() << qPrintable(QString("The server is now running at: %1:%2").arg(ipString).arg(serverPort()));
    quizFile = new QFile(ipDiscoveryPath+"Quiz.txt");
    quizFile->open(QIODevice::WriteOnly | QIODevice::Truncate);
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
    out << serverPort();
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
                    int total = quizSheet->read(1, 1).toInt();
                    QXlsx::Worksheet *questionSheet = dynamic_cast<QXlsx::Worksheet *>(xlsxq.sheet(name));
                    int newTotal;
                    for (newTotal = 1; newTotal <= questionSheet->dimension().rowCount(); newTotal++) {
                        if (questionSheet->read(newTotal, 1).toString() == "") {
                            break;
                        }
                    }
                    newTotal -= 2; // Ignore first row
                    // Verify we have as many questions in this sheet as we did last time we loaded it
                    if (total != newTotal) {
                        if (total > newTotal) {
                            qDebug() << "The sheet " << name << " has less questions than it did before. This may cause issues.\nPlease fix this. Before: " << total << " Now: " << newTotal;
                        } else {
                            qDebug() << "The sheet " << name << " has more questions than it did before. Extending answer sheet. Before: " << total << " Now: " << newTotal;
                            // Clear column 2 + total (answers need to go here)
                            for (int r = 1; r <= quizSheet->dimension().rowCount(); r++) {
                                quizSheet->writeBlank(r, 2 + total);
                            }
                            // Write column 2 + newTotal
                            quizSheet->writeString(2, 2 + newTotal, "Total", boldFormat);
                            quizSheet->writeFormula(4, 2 + newTotal, QXlsx::CellFormula("=SUM(B4:" + QXlsx::CellReference(2, 1 + newTotal).toString() + ")"), boldFormat);
                            for (int r = 6; r <= quizSheet->dimension().rowCount(); r++) {
                                if (quizSheet->read(r, 1).toString() != "") {
                                    // If student exists, write the sumproduct
                                    quizSheet->writeFormula(r, 2 + newTotal, QXlsx::CellFormula("=SUMPRODUCT(--(" + QXlsx::CellReference(r, 2).toString(false, true) + ":" + QXlsx::CellReference(r, 1 + newTotal).toString(false, true) + "=$B$3:" + QXlsx::CellReference(3, 1 + newTotal).toString(true, true) +  "))"));
                                }
                            }
                            // Fill in gaps
                            for (int c = total; c <= newTotal; c++) {
                                quizSheet->writeString(2, c + 1, QString::number(c), boldFormat);
                                quizSheet->writeFormula(4, c + 1, QXlsx::CellFormula("=COUNTIF(" + QXlsx::CellReference(5, c + 1).toString(true, false) + ":OFFSET(" + QXlsx::CellReference(5, c + 1).toString(true, false) + ",$B$1,0)," + QXlsx::CellReference(3, c + 1).toString(true, false) + ")"), boldFormat);
                            }
                            // Just replace all answers, just in case questions were updated too!
                            for (int a = 1; a <= newTotal; a++) {
                                quizSheet->writeString(3, a + 1, questionSheet->cellAt(a + 1, 3)->value().toString());
                            }
                            // We need code to update what happens to the sheets once the questions have changed?
                            // We can assume no real students have taken the test at this stage.

                            // Make every student back to status 1 (if they were 2).
                            for (int r = 2; r <= summarySheet->dimension().rowCount(); r++) {
                                QString curStatus = summarySheet->read(r, i).toString();
                                if (curStatus.length() && curStatus.at(0) == '2') {
                                    summarySheet->writeString(r, i, QString("1,%2").arg(total));
                                }
                            }
                            // Update total so we know we don't need to update this again
                            quizSheet->writeString(1, 1, QString::number(newTotal));

                            // Fix widths
                            quizSheet->setColumnWidth(2, 1 + newTotal, 5.0);
                            quizSheet->setColumnWidth(2 + newTotal, 2 + newTotal, 6.0);

                            // Save
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
                if (!(name.startsWith("Card"))) {
                    for (int j = 2; j <= summarySheet->dimension().rowCount(); j++) {
                        // For each student, add the quiz score as status 0, position 0
                        summarySheet->write(j, i, "0,0");
                    }
                    // TODO: Scan through user records to see if this card was required.
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
            qDebug() << "User " << username << " has connected.";
            block = readExcelDatabase(username);
        }
    } else if (command == "question") {
        QString quizName;
        quint16 question;
        in >> quizName >> question;
        block = sendUserQuestion(quizName, question);
    } else if (command == "update" || command == "updatelast") {
        if (!(username.isEmpty())) {
            qDebug() << "User " << username << " has answered a question.";
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

            block = updateUserAnswer(username, quizName, question, answer, currentQuiz);
        }
    } else {
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);
        out << (quint32)0 << QString("Error");
    }
    // Write first byte as size and send block
    QDataStream out (&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out.device()->seek(0);
    out << (quint32)(block.size() - sizeof(quint32));
    tcpSocket.write(block);

    // Now we will disconnect from client.
    tcpSocket.disconnectFromHost();
    if (tcpSocket.state() == QAbstractSocket::UnconnectedState ||
            tcpSocket.waitForDisconnected(500))
        tcpSocket.close();
}

QStringList ServerThread::listOfRequiredCards(QString quizName, QString user) {
    QXlsx::Document xlsx(xlsxPath+"Results.xlsx");
    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));
    QXlsx::CellRange range = quizSheet->dimension();
    QStringList cards = QStringList();
    int total = quizSheet->read(1,1).toInt();
    for (int row = 6; row <= range.lastRow(); row++) {
        QString name = quizSheet->read(row, 1).toString();
        if (name == user) {
            for (int c = 2; c <= total + 1; c++) {
                QString card = quizSheet->read(5, c).toString();
                // Invalid card?
                if (card == "" || cards.contains(card))
                    continue;
                // If user answer matches real answer, add the card
                if (quizSheet->read(row, c).toString() == quizSheet->read(3, c).toString())
                    cards.append(card);
            }
        }
    }
    std::sort(cards.begin(),cards.end());
    return cards;
}

// Using the QtXlsx third-party plugin, we can manipulate Excel 2007+ files from right here.
QByteArray ServerThread::readExcelDatabase(QString user) {
    // Begin block writing for C++ model
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << (quint32)0;

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
                if (quiz.startsWith("Card")) {
                    // This is a 'card', which we use as a conditional quiz.
                    // It is conditional on the results of the quiz.
                    if (summarySheet->read(row, col).toString() == "") {
                        continue;
                    }
                }

                QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quiz));
                if (quizSheet == nullptr) {
                    createQuizSheet(&xlsx, quiz);
                    quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quiz));
                }
                total = quizSheet->read(1, 1).toInt();
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
                QString quiz = summarySheet->read(1, col).toString();
                if (quiz.startsWith("Card")) {
                    continue;
                }

                summarySheet->writeString(row, col, "0,0");

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
    int total;
    for (total = 1; total <= questionSheet->dimension().rowCount(); total++) {
        if (questionSheet->read(total, 1).toString() == "") {
            break;
        }
    }
    total -= 2;
    if (total <= 0)
        total = 1; // Otherwise structure doesn't work and Excel will bug!
    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx->sheet(quizName));
    bool hasCards = !(quizName.startsWith("Card"));

    quizSheet->setRowHidden(1, 1, true); // Hide the row that has # of questions and # of students
    quizSheet->writeString(1, 1, QString::number(total));
    quizSheet->writeFormula(1, 2, QXlsx::CellFormula(QString("=COUNTA(A:A)-%1").arg(hasCards ? 5 : 4)));
    QXlsx::Format boldFormat;
    boldFormat.setFontBold(true);
    // Count Row
    quizSheet->writeString(2, 1, "Students", boldFormat);
    for (int i = 1; i <= total; i++) {
        quizSheet->writeString(2, 1 + i, QString::number(i), boldFormat);
    }
    quizSheet->writeString(2, 2 + total, "Total", boldFormat);

    // Answer Row
    quizSheet->writeString(3, 1, "Answer", boldFormat);
    for (int i = 1; i <= total; i++) {
        quizSheet->writeString(3, 1 + i, questionSheet->read(1 + i, 3).toString());
    }

    // Total Row
    quizSheet->writeString(4, 1, "Total", boldFormat);
    for (int i = 1; i <= total; i++) {
        quizSheet->writeFormula(4, 1 + i, QXlsx::CellFormula("=COUNTIF(" + QXlsx::CellReference(6, 1 + i).toString(true, false) + ":OFFSET(" + QXlsx::CellReference(6, 1 + i).toString(true, false) + ",$B$1,0)," + QXlsx::CellReference(3, 1 + i).toString(true, false) + ")"), boldFormat);
    }
    quizSheet->writeFormula(4, 2 + total, QXlsx::CellFormula("=SUM(B4:" + QXlsx::CellReference(4, 1 + total).toString() + ")"), boldFormat);

    // Card Row
    if (hasCards) {
        quizSheet->writeString(5, 1, "Card", boldFormat);
        for (int i = 1; i <= total; i++) {
            quizSheet->writeString(5, 1 + i, questionSheet->read(1 + i, 4).toString());
        }
    }

    // Fix widths
    quizSheet->setColumnWidth(1, 1, 12.0); // Student names
    quizSheet->setColumnWidth(2, 1 + total, 5.0); // Set widths for counts/answers
    quizSheet->setColumnWidth(2 + total, 2 + total, 6.0); // Answer text

    // Save
    xlsx->save();
}

QByteArray ServerThread::updateUserAnswer(QString username, QString quizName, int question, QString answer, quint16 currentQuiz) {
    QXlsx::Document xlsx(xlsxPath+"Results.xlsx");

    QXlsx::Worksheet *quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));

    QByteArray block;
    if (quizSheet == nullptr) {

        createQuizSheet(&xlsx, quizName);
        quizSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));
    }

    // We need to know total to work out if we finished this quiz.
    int total = quizSheet->read(1,1).toInt();
    int correct = 0;

    QXlsx::CellRange range = quizSheet->dimension();
    for (int row = 5; row <= range.lastRow() + 1; row++) {
        if (quizSheet->read(row, 1).toString() == username) {
            quizSheet->writeString(row, 1 + question, answer);
            // QXlsx cannot do SUMPRODUCT until saved by Excel. So we can't just check the formula.
            for (int col = 2; col < 2 + total; col++) {
                if (quizSheet->read(row, col).toString() == quizSheet->read(3, col).toString())
                    correct++;
            }
            //correct = quizSheet->cellAt(row, 2 + total)->value().toInt();
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
                    if (question == total) {
                        // If we are finished we need to update status and show number of correct
                        summarySheet->writeString(row, col, QString("2,%1").arg(QString::number(correct)));
                    } else {
                        // Otherwise, we are just showing current question number
                        summarySheet->writeString(row, col, QString("1,%1").arg(QString::number(question)));
                    }
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
    } else {
        // This is the last question in the quiz.
        // We will give them how many were correct instead of next question.
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_4);
        out << (quint32)0 << currentQuiz << (quint16)correct;

        // Check if we have to send them new card details
        if (!(quizName.startsWith("Card"))) {
            // This is a proper quiz and we need to work out the cards
            QStringList cards = listOfRequiredCards(quizName, username);
            for (int row = 2; row <= range.lastRow(); row++) {
                QString name = summarySheet->read(row, 1).toString();
                if (name == username) {
                    for (int c = 2; c <= range.lastColumn(); c++) {
                        if (summarySheet->read(row, c).toString() != "")
                            continue; // Already have this card!
                        QString card = summarySheet->read(1, c).toString();
                        if (card == "")
                            continue;
                        QString cardCompare = card;
                        cardCompare.remove("Card ");
                        if (cards.contains(cardCompare)) {
                            // Bingo
                            summarySheet->writeString(row, c, "0,0");
                            QXlsx::Worksheet *cardSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(card));
                            if (cardSheet == nullptr) {
                                createQuizSheet(&xlsx, card);
                                cardSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(card));
                            }
                            qint16 total = cardSheet->read(1, 1).toInt();
                            out << card << total;
                        }
                    }
                }
            }
        }
        // Save the file
        xlsx.save();
    }

    return block;
}

QByteArray ServerThread::sendUserQuestion(QString quizName, int question)
{
    // Begin block writing for C++ model
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_4);
    out << (quint32)0;
    QXlsx::Document xlsx(xlsxPath+"Questions.xlsx");
    QXlsx::Worksheet *questionSheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(quizName));
    bool sa = true;
    QString questionStr = questionSheet->read(1 + question, 1).toString();
    // Add quiz folder to Img_ to make cards easier to write.
    QString quizFolder = quizName;
    quizFolder.replace(".","").replace(" ","");
    questionStr.replace("{Img_", QString("{Img_%1/").arg(quizFolder));
    QString answers = questionSheet->read(1 + question, 2).toString();
    answers.replace("{Img_", QString("{Img_%1/").arg(quizFolder));
    if (answers.contains(','))
        sa = false;
    // Type: MC (0) or SA (1)
    // Question and then Answers list
    out << sa << questionStr << answers.split(',');

    return block;
}
