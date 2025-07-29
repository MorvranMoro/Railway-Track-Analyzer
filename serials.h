#ifndef SERIALS_H
#define SERIALS_H

#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include "struct.h"
#include <iomanip>
#include <fstream>
#include <string>
#include <windows.h>
#include <commdlg.h>
#include <sstream>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <math.h>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>
#include <QPushButton>

using namespace std;

double median(vector<double> &Ey)
{
    std::sort(Ey.begin(), Ey.end());

    int N = Ey.size();

    if (N % 2 == 0)
    {
        return (Ey[N / 2 - 1] + Ey[N / 2]) / 2.0;
    }
    else
    {
        return Ey[N / 2];
    }
}

solv_struct serialss(double size, int const sector)
{
    // Диалог выбора источника данных
    QMessageBox msgBox;
    msgBox.setText("Выберите источник данных");
    QPushButton *fileButton = msgBox.addButton("Из файла", QMessageBox::ActionRole);
    QPushButton *dbButton = msgBox.addButton("Из базы данных", QMessageBox::ActionRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.exec();

    QVector<double> Ex;
    QString tableName;
    int minId = -1;

    if (msgBox.clickedButton() == fileButton) {
        // Загрузка из файла
        QString filePath = QFileDialog::getOpenFileName(nullptr, "Выберите файл", "", "Text Files (*.txt)");
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Не удалось открыть файл для чтения";
            return solv_struct();
        }

        QTextStream in(&file);
        while (!in.atEnd() && Ex.size() < size) {
            double value;
            in >> value;
            Ex.append(value);
        }
        file.close();
    } else if (msgBox.clickedButton() == dbButton) {
        // Загрузка из базы данных
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            qDebug() << "Нет подключения к базе данных";
            return solv_struct();
        }

        // Диалог выбора таблицы и столбца
        QStringList tables = db.tables(QSql::Tables);
        tableName = QInputDialog::getItem(nullptr, "Выбор таблицы",
                                          "Выберите таблицу:", tables, 0, false);

        if (tableName.isEmpty()) return solv_struct();

        // Получаем minId из базы данных
        QSqlQuery idQuery(db);
        if (!idQuery.exec("SELECT MIN(id) FROM " + tableName)) {
            qDebug() << "Ошибка получения minId:" << idQuery.lastError().text();
            return solv_struct();
        }

        if (idQuery.next()) {
            minId = idQuery.value(0).toInt();
            qDebug() << "minId из базы данных: " << minId;
        } else {
            qDebug() << "В таблице нет данных для получения minId";
            return solv_struct();
        }

        QSqlQuery columnQuery(db);
        if (!columnQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = '" +
                              tableName + "'")) {
            qDebug() << "Ошибка получения столбцов:" << columnQuery.lastError().text();
            return solv_struct();
        }

        QStringList columns;
        while (columnQuery.next()) {
            columns << columnQuery.value(0).toString();
        }

        QString columnName = QInputDialog::getItem(nullptr, "Выбор столбца",
                                                   "Выберите столбец с данными:", columns, 0, false);

        if (columnName.isEmpty()) return solv_struct();

        // Получаем данные из выбранного столбца
        Ex.clear();
        Ex.reserve(size);

        for (int i = 0; i < size; ++i) {
            QSqlQuery query(db);
            query.prepare("SELECT " + columnName + " FROM " + tableName + " WHERE id = ?");
            query.addBindValue(minId + i);

            if (!query.exec()) {
                qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
                Ex.append(0.0);
                continue;
            }

            if (query.next()) {
                QVariant value = query.value(0);
                bool ok;
                double doubleValue = value.toDouble(&ok);

                if (ok) {
                    Ex.append(doubleValue);
                } else {
                    qDebug() << "Ошибка преобразования значения (id=" << minId + i << "): " << value;
                    Ex.append(0.0);
                }
            } else {
                qDebug() << "Нет данных для id = " << minId + i;
                Ex.append(0.0);
            }
        }
    } else {
        return solv_struct();
    }

    // Проверка, что данные получены
    if (Ex.isEmpty()) {
        qDebug() << "Нет данных для анализа";
        return solv_struct();
    }

    double N = size;
    vector<double> Ur(sector);
    vector<double> Qr(sector);
    vector<double> SUr(sector);
    vector<double> SQr(sector);
    vector<double> SrZnacnMas(sector);
    int Sr = 1.0;
    int SrZnacnMas_Sr = 1.0;
    int Sector12 = sector;
    int zona = N / Sector12;
    double SrCvadr = 0.0;
    double SrZnacn = 0.0;

    // Вычисление средних значений для каждого сектора
    for (int i = 0; i < Sector12; i++) {
        SrZnacn = 0;
        for (int j = 0; j < zona - 1; j++) {
            SrZnacn += Ex.at(zona * i + j);
        }
        SrZnacnMas[i] = SrZnacn / (zona - 1);
    }

    // Вычисление дисперсии для каждого сектора
    for (int i = 0; i < Sector12; i++) {
        SrCvadr = 0;
        for (int j = 0; j <= zona; j++)
            SrCvadr += ((Ex.at(zona * i + j) - SrZnacnMas[i]) * (Ex.at(zona * i + j) - SrZnacnMas[i]));
        Qr[i] = (SrCvadr / (zona-1));
    }

    SQr = Qr;
    SUr = SrZnacnMas;
    double Msr = median(SUr);
    double MQr = median(SQr);

    // Подсчет серий для дисперсии и средних значений
    for (int Na = 0; Na < Sector12 - 1; Na++) {
        if ((Qr[Na] > MQr) && (Qr[Na + 1] < MQr)) Sr++;
        else if ((Qr[Na] < MQr) && (Qr[Na + 1] > MQr)) Sr++;

        if ((SrZnacnMas[Na] > Msr) && (SrZnacnMas[Na + 1] < Msr)) SrZnacnMas_Sr++;
        else if ((SrZnacnMas[Na] < Msr) && (SrZnacnMas[Na + 1] > Msr)) SrZnacnMas_Sr++;
    }

    solv_struct tmp;
    tmp.Ur = SrZnacnMas;
    tmp.Qr = Qr;
    tmp.Bobby = Sr;      // Количество серий для дисперсии
    tmp.Bobby2 = SrZnacnMas_Sr; // Количество серий для средних значений

    QMessageBox saveMsgBox;
    saveMsgBox.setText("Сохранить результаты в базу данных?");
    saveMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (saveMsgBox.exec() == QMessageBox::Yes) {
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            qDebug() << "Нет подключения к базе данных";
            return tmp;
        }

        // Если таблица не выбрана (при загрузке из файла), запрашиваем
        if (tableName.isEmpty()) {
            QStringList tables = db.tables(QSql::Tables);
            tableName = QInputDialog::getItem(nullptr, "Выбор таблицы",
                                              "Выберите таблицу для сохранения:", tables, 0, false);
            if (tableName.isEmpty()) return tmp;
        }

        // Получаем список столбцов для сохранения
        QSqlQuery query(db);
        if (!query.exec("SELECT column_name FROM information_schema.columns WHERE table_name = '" +
                        tableName + "'")) {
            qDebug() << "Ошибка получения столбцов:" << query.lastError().text();
            return tmp;
        }

        QStringList columns;
        while (query.next()) {
            columns << query.value(0).toString();
        }

        // Сохраняем результаты в разные столбцы
        QString saveColumnQr = QInputDialog::getItem(nullptr, "Выбор столбца",
                                                   "Выберите столбец для сохранения Qr:",
                                                   columns, 0, false);

        QString saveColumnUr = QInputDialog::getItem(nullptr, "Выбор столбца",
                                                   "Выберите столбец для сохранения Ur:",
                                                   columns, 0, false);

        if (saveColumnQr.isEmpty() || saveColumnUr.isEmpty()) return tmp;

        // Сохраняем результаты
        db.transaction();
        try {
            // Получаем минимальный и максимальный ID
            QSqlQuery idQuery(db);
            if (!idQuery.exec("SELECT MIN(id), MAX(id) FROM \"" + tableName + "\"")) {
                qDebug() << "Ошибка получения ID:" << idQuery.lastError().text();
                db.rollback();
                return tmp;
            }
            idQuery.next();
            int minId = idQuery.value(0).toInt();
            int maxId = idQuery.value(1).toInt();

            // Сохраняем Qr
            QSqlQuery updateQrQuery(db);
            updateQrQuery.prepare("UPDATE \"" + tableName + "\" SET \"" + saveColumnQr + "\" = :value WHERE id = :id");
            for (int i = 0; i < Qr.size(); ++i) {
                updateQrQuery.bindValue(":value", Qr[i]);
                updateQrQuery.bindValue(":id", minId + i);
                if (!updateQrQuery.exec()) {
                    db.rollback();
                    qDebug() << "Ошибка сохранения Qr:" << updateQrQuery.lastError().text();
                    return tmp;
                }
            }

            // Сохраняем Ur
            QSqlQuery updateUrQuery(db);
            updateUrQuery.prepare("UPDATE \"" + tableName + "\" SET \"" + saveColumnUr + "\" = :value WHERE id = :id");
            for (int i = 0; i < SrZnacnMas.size(); ++i) {
                updateUrQuery.bindValue(":value", SrZnacnMas[i]);
                updateUrQuery.bindValue(":id", minId + i);
                if (!updateUrQuery.exec()) {
                    db.rollback();
                    qDebug() << "Ошибка сохранения Ur:" << updateUrQuery.lastError().text();
                    return tmp;
                }
            }

            db.commit();
            qDebug() << "Результаты успешно сохранены в столбцы" << saveColumnQr << "и" << saveColumnUr;
        } catch (...) {
            db.rollback();
            qDebug() << "Ошибка при сохранении результатов";
        }
    }

    return tmp;
}

#endif // SERIALS_H
