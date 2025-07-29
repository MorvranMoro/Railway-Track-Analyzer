QT       += core gui
QT       += charts
QT       += core gui sql
QT       += sql
QT       += sql network  # Добавляем поддержку SQL и сетевых функций (для SSL)
LIBS     += -L"C:/Program Files/PostgreSQL/15/lib" -llibpq

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    databaseManager.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Cepstral.h \
    correl.h \
    crosscorr.h \
    databaseManager.h \
    inversions.h \
    mainwindow.h \
    serials.h \
    specanal.h \
    struct.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
