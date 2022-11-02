QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/scene.cpp \
    src/view.cpp

HEADERS += \
    src/polygon_point.h \
    src/cell.h \
    src/constants.h \
    src/edge.h \
    src/scene.h \
    src/view.h

RESOURCES += \
    resource.qrc
