#include <QCoreApplication>
#include "server.h"

// The purpose of this server is to act as a communication gateway between student UI and teacher Excel.
// The students ('client') are able to talk to the server to enter their results.
// Then the teacher ('server') is able to record the result in to an Excel file in a private area.
// The private area can only be accessed by teachers so as to prevent tampering.
// An excel format is chosen as the database as it allows easy access to viewing and graphing of results.


// We wish to remove this file afterwards.
// This file is chosen as it is in a location that students and teachers can access.
// This is a simple way of giving the students the IP address of the server without hardcoding.
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

