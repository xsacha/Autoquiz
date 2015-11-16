#ifndef ACCOUNTINFO_H
#define ACCOUNTINFO_H

#include <QObject>
#include <QDebug>
#include "client.h"

class AccountInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString user MEMBER _user NOTIFY userChanged)
    Q_PROPERTY(bool    loggedin MEMBER _loggedin NOTIFY loggedinChanged)
public:
    explicit AccountInfo(QObject *parent = 0);

signals:
    void userChanged(const QString &newUser);
    void loggedinChanged(const bool &loggedin);

public slots:

private:
    QString _user;
    bool    _loggedin;
    Client* client;
};

#endif // ACCOUNTINFO_H
