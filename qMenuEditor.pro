QT += core gui widgets xml

CONFIG += c++17

TARGET = qMenuEditor
TEMPLATE = app

# Rutas de salida organizadas dentro de build/
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
    $$PWD/src/ui/MainWindow.cpp

HEADERS += \
    $$PWD/src/core/DesktopEntry.h \
    $$PWD/src/ui/MainWindow.h

# Permite que el binario en /opt/qMenuEditor/bin/ cargue librerías desde /opt/qMenuEditor/libs/
QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN/../libs\'

# Destino base de instalación
isEmpty(PREFIX) {
    PREFIX = /opt/qMenuEditor
}

# Ejecutable en /opt/qMenuEditor/bin/
target.path = $$PREFIX/bin
INSTALLS += target

# Iconos de la aplicación en /opt/qMenuEditor/icons/ (si existen en resources/icons)
exists($$PWD/resources/icons) {
    app_icons.path = $$PREFIX/icons
    app_icons.files = $$PWD/resources/icons/*
    INSTALLS += app_icons
}
