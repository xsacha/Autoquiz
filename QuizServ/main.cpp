#include <QCoreApplication>
#include <QNetworkInterface>
#include <QTemporaryFile>
#include <QDataStream>
#include "server.h"

// The purpose of this server is to act as a communication gateway between student UI and teacher Excel.
// The students ('client') are able to talk to the server to enter their results.
// Then the teacher ('server') is able to record the result in to an Excel file in a private area.
// The private area can only be accessed by teachers so as to prevent tampering.
// An excel format is chosen as the database as it allows easy access to viewing and graphing of results.


void func() {
#ifdef WINVER
    QFile::remove("\\\\10.113.28.3\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
#else
    QFile::remove("./Quiz.txt"); //?
#endif
}

int main(int argc, char *argv[])
{
    std::atexit(func);
    QCoreApplication a(argc, argv);
    a.setApplicationName("Quiz Server");

    QScopedPointer<Server> s(new Server(&a));

    return a.exec();
}
