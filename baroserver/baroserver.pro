#-------------------------------------------------
#
# Project created by QtCreator 2015-05-08T06:58:49
#
#-------------------------------------------------

QT       += core websockets serialport

QT       -= gui

TARGET = baroserver
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    server.cpp \
    barometer.cpp

LIBS += -L$$PWD/../../QRealFourier/code/build/ -lqrealfourier

INCLUDEPATH += $$PWD/../../QRealFourier/code/build $$PWD/../../QRealFourier/code/headers
DEPENDPATH += $$PWD/../../QRealFourier/code/build

HEADERS += \
    server.h \
    barometer.h
