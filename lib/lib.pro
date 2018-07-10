TEMPLATE = lib
TARGET = skedplayer
CONFIG += staticlib
CONFIG += c++11
#CONFIG += debug

# Build directory
OBJECTS_DIR = ../build
MOC_DIR     = ../build
RCC_DIR     = ../build
UI_DIR      = ../build
DESTDIR     = ../build

SOURCES += skedplayer.cpp
HEADERS += skedplayer.h

STAGING_DIR = $(STAGING_DIR)
message(STAGING_DIR: $${STAGING_DIR})

INCLUDEPATH += $${STAGING_DIR}/usr/include
INCLUDEPATH += $${STAGING_DIR}/usr/include/aliplatform
LIBS += -L$${STAGING_DIR}/usr/lib -lalislsnd -laui

headers.files = skedplayer.h
headers.path  = /usr/include
target.path = /usr/lib
INSTALLS += target headers
