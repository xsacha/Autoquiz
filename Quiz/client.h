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
    Q_PROPERTY(QAbstractListModel* model READ model() NOTIFY modelChanged)
    explicit Client(QObject *parent = 0);
    AccountInfo* accountInfo;

    QAbstractListModel* model() const {
        return _model;
    }

public slots:
    void sendLogin();
    void requestDetails();
    void readDetails();
    void displayError(QAbstractSocket::SocketError socketError);

signals:
    void loggedinChanged(const bool &loggedin);
    void modelChanged();

private:
    QString _username;
    bool    _connected = false;
    qint16  _blockSize;
    bool    _loggedin;
    QuizModel* _model;
};

#endif // CLIENT_H
