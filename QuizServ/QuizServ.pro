QT += core network sql
QT -= gui

TARGET = QuizServ
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp

HEADERS += \
    server.h

DEFINES += XLSX_NO_LIB
include(QtXlsxWriter/src/xlsx/qtxlsx.pri)
