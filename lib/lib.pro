TEMPLATE = lib
TARGET = skedplayer
CONFIG += staticlib
CONFIG += c++11
#CONFIG += debug
QT -= gui

# Build directory
OBJECTS_DIR = ../build
MOC_DIR     = ../build
RCC_DIR     = ../build
UI_DIR      = ../build
DESTDIR     = ../build

SOURCES += skedplayer.cpp
HEADERS += skedplayer.h

LIBS += -laui

headers.files = skedplayer.h
headers.path  = /usr/include
target.path = /usr/lib
INSTALLS += target headers
