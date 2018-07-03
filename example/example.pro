TEMPLATE = app
TARGET = skedplayer
QTPLUGIN += qdirectfb qevdevkeyboardplugin
QT += quick widgets
CONFIG += c++11
CONFIG += debug

# Build directory
OBJECTS_DIR = build
MOC_DIR     = build
RCC_DIR     = build
UI_DIR      = build
DESTDIR     = build

SOURCES += example.cpp
RESOURCES += qml.qrc

STAGING_DIR = $$(STAGING_DIR)
message(STAGING_DIR: ${{STAGING_DIR}})

INCLUDEPATH += $${STAGING_DIR}/usr/include
LIBS += -L../build -lskedplayer
LIBS += -L$${STAGING_DIR}/usr/qml/QtQuick.2 -lqtquick2plugin -lnmpgoplayer -lalislsnd -laui

target.path = /opt/$${TARGET}/bin
INSTALLS += target
