#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QThread>
#include <QFile>
#include <xlsxdocument.h>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);
    ~Server() { quizFile->remove(); delete quizFile; }
    QString ipDiscoveryPath;

protected:
    void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE;

private:
    QFile* quizFile;
    QString xlsxPath;
};

class ServerThread : public QThread
{
    Q_OBJECT

public:
    ServerThread(int socketDescriptor, QString path, QObject *parent);
    QByteArray readExcelDatabase(QString user);
    void createSummarySheet(QXlsx::Document *xlsx);
    void createQuizSheet(QXlsx::Document *xlsx, QString quizName);
    QByteArray updateUserAnswer(QString username, QString quizName, int question, QString answer, quint16 currentQuiz);
    QByteArray sendUserQuestion(QString quizName, int question);
    QString capturedToPath(QString captured);

    void run() Q_DECL_OVERRIDE;

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    int socketDescriptor;
    QString xlsxPath;
};

#endif // SERVER_H
