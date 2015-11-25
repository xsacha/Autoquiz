#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>
#include "accountinfo.h"

class Client : public QTcpSocket
{
    Q_OBJECT
public:
    Q_PROPERTY(bool    loggedin MEMBER _loggedin NOTIFY loggedinChanged)
    Q_PROPERTY(bool    sending MEMBER  _sending NOTIFY sendingChanged)
    Q_PROPERTY(bool    curType MEMBER  _curType NOTIFY questionChanged)
    Q_PROPERTY(QString curQuestion MEMBER  _curQuestion NOTIFY questionChanged)
    Q_PROPERTY(QStringList curAnswers MEMBER  _curAnswers NOTIFY questionChanged)
    Q_PROPERTY(QAbstractListModel* model READ model() NOTIFY modelChanged)
    explicit Client(QObject *parent = 0);
    AccountInfo* accountInfo;

    QAbstractListModel* model() const {
        return _model;
    }

public slots:
    void sendData();
    Q_INVOKABLE void updateDetails(int currentQuiz, QString quizName, quint16 position, QString value);
    Q_INVOKABLE void requestDetails();
    Q_INVOKABLE void requestQuestion(QString quizName, quint16 question);
    void readResponse();
    void startConnection();
    void displayError(QAbstractSocket::SocketError socketError);

signals:
    void loggedinChanged();
    void modelChanged();
    void sendingChanged();
    void questionChanged();

private:
    QString _username;
    bool    _connected;
    qint16  _blockSize;
    bool    _loggedin;
    QuizModel* _model;
    bool    _sending;
    QByteArray dataPacket;
    bool _curType;
    QString _curQuestion;
    QStringList _curAnswers;
};

#endif // CLIENT_H
