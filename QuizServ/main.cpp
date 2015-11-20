#include <QCoreApplication>
#include "server.h"


void removeFile() {
    QFile::remove("\\\\eqsun2102003\\Data\\Curriculum\\Common\\Maths\\Quiz.txt");
}

int main(int argc, char *argv[])
{
    atexit(removeFile);
    QCoreApplication a(argc, argv);
    a.setApplicationName("Quiz Server");

    Server* s = new Server(&a);
    Q_UNUSED(s)

    return a.exec();
}

