TEMPLATE = app
QT += core dbus
TARGET = skedplayer-server
CONFIG += c++11
QT -= gui

# Build directory
OBJECTS_DIR = build
MOC_DIR     = build
RCC_DIR     = build
UI_DIR      = build
DESTDIR     = build

SOURCES += main.cpp
HEADERS += skedplayer-server.h

LIBS += -L../build -lskedplayer
LIBS += -laui

target.path = /opt/sked/player/bin
dbus.files  = com.sked.service.player.conf
dbus.path   = /etc/dbus-1/system.d
INSTALLS   += dbus target
