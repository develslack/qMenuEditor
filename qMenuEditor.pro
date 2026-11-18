QT += core gui widgets xml

CONFIG += c++17

TARGET = qMenuEditor
TEMPLATE = app

DESTDIR     = $$PWD/build
OBJECTS_DIR = $$PWD/build/.obj
MOC_DIR     = $$PWD/build/.moc
UI_DIR      = $$PWD/build/.ui
RCC_DIR     = $$PWD/build/.rcc

INCLUDEPATH += $$PWD/src \
               $$PWD/src/core \
               $$PWD/src/modules \
               $$PWD/src/ui

SOURCES += \
    $$PWD/src/main.cpp \
    $$PWD/src/core/DesktopEntry.cpp \
    $$PWD/src/modules/MenuAuditor.cpp \
    $$PWD/src/ui/IconPickerDialog.cpp \
    $$PWD/src/ui/MenuAuditDialog.cpp \
    $$PWD/src/ui/AboutDialog.cpp \
    $$PWD/src/ui/MainWindow.cpp

HEADERS += \
    $$PWD/src/core/DesktopEntry.h \
    $$PWD/src/modules/MenuAuditor.h \
    $$PWD/src/ui/IconPickerDialog.h \
    $$PWD/src/ui/MenuAuditDialog.h \
    $$PWD/src/ui/AboutDialog.h \
    $$PWD/src/ui/MainWindow.h

QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN/../libs\'

isEmpty(PREFIX) {
    PREFIX = /opt/qMenuEditor
}

target.path = $$PREFIX/bin
INSTALLS += target

# Instalar documentación y licencias en /opt/qMenuEditor/documentation/
exists($$PWD/documentation) {
    app_docs.path = $$PREFIX/documentation
    app_docs.files = $$PWD/documentation/*
    INSTALLS += app_docs
}
