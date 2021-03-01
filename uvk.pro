QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


DEFINES += NO_LIBRARY
MVP_ROOT=$$PWD/../..

#https://github.com/yx500/mvp_classes.git
include($$MVP_ROOT/mvp_classes/mvp_model_gorka.pri)
#https://github.com/yx500/common_src.git
include($$MVP_ROOT/common_src/signalmanager/signalmanager.pri)
include($$MVP_ROOT/common_src/gtcommandinterface/gtcommandinterface.pri)


include($$PWD/tracking_otceps/tracking_otceps.pri)


SOURCES += \
        main.cpp \
    gtgac.cpp \
    otcepscontroller.cpp \
    uvk_central.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Crc32.h \
    messageDO.h \
    gtapp_watchdog.h \
    gtgac.h \
    otcepscontroller.h \
    uvk_central.h
