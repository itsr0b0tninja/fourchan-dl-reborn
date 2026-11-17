QT += core network
QT -= gui
TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
TARGET = tls-smoke
DESTDIR = ../portable
SOURCES += tls-smoke.cpp
