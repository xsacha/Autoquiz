#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QAbstractSocket>

class Client : public QTcpSocket
{
    Q_OBJECT
public:
    explicit Client(QString user, QObject *parent = 0);

public slots:
    void sendLogin();
    void requestDetails();
    void readDetails();
    void displayError(QAbstractSocket::SocketError socketError);

private:
    QString _username;
    bool _connected = false;
    qint16 _blockSize;
};

#endif // CLIENT_H
