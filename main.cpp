#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QtSql>
#include <QDebug>
#include <QMessageBox>
#include <QLibrary>

int main(int argc, char *argv[])
{


    QApplication a(argc, argv);
    MainWindow w;
    QLibrary sslLib("libssl-3-x64");
    QLibrary cryptoLib("libcrypto-3-x64");

    if (!sslLib.load() || !cryptoLib.load()) {
        qDebug() << "OpenSSL load error:" << sslLib.errorString() << cryptoLib.errorString();
        return 1;
    }

    QCoreApplication::addLibraryPath("C:/Qt/6.8.0/msvc2022_64/plugins");
    w.show();
    return a.exec();


}
