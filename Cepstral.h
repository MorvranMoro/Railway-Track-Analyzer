#ifndef CEPSTRAL_H
#define CEPSTRAL_H

#pragma once

#include <iostream>
#include <vector>
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

solv_struct ceps(double size)
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
    int minId = -1; // Инициализируем minId значением по умолчанию

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

        // Получаем данные из выбранного столбца, используя ID в качестве ориентира
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
    std::vector<double> X(N);
    std::vector<double> Aq(N/2.0);
    std::vector<double> Bq(N/2.0);
    std::vector<double> Pq(N/2.0);
    double E = 0;

    std::vector<double> C(N/2.0);
    std::vector<double> Ac(N/2.0);
    std::vector<double> Bc(N/2.0);
    std::vector<double> Pc(N/2.0);

    for(int i = 0; i < N/2.0; i++){
        C[i] = log(abs(Ex[i]));
    }

    for (int q = 1; q < N/4.0; q++) {
        E = 0;
        for (int i = 1; i < N/2.0; i++) {
            E = E + C[i] * (cos((2 * 3.14 * q * i) / N/2.0));
        }
        Ac[q] = E * 2 / N;
    }

    for (int q = 1; q < N/4.0; q++) {
        E = 0;
        for (int i = 1; i < N/2.0; i++) {
            E = E + C[i] * (sin((2.0 * 3.14 * q * i) / N/2.0));
        }
        Bc[q] = E * 2.0 / N;
    }

    for (int i = 1; i < N/4.0; i++) {
        Pc[i-1] = Ac[i] * Ac[i] + Bc[i] * Bc[i];
    }

    solv_struct tmp;
    tmp.veccor1 = Pc;

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

        QString saveColumn = QInputDialog::getItem(nullptr, "Выбор столбца",
                                                   "Выберите столбец для сохранения результатов:",
                                                   columns, 0, false);

        if (saveColumn.isEmpty()) return tmp;

        // Сохраняем результаты, начиная с первой строки, с использованием id
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
            qDebug() << "minId:" << minId << "maxId:" << maxId;

            // Подготавливаем запрос один раз перед циклом
            QSqlQuery updateQuery(db);
            updateQuery.prepare("UPDATE \"" + tableName + "\" SET \"" + saveColumn + "\" = :value WHERE id = :id");

            for (int i = 0; i < Pc.size(); ++i) {
                updateQuery.bindValue(":value", Pc[i]);
                updateQuery.bindValue(":id", minId + i);

                qDebug() << "Setting value" << Pc[i] << "for id" << (minId + i);

                if (!updateQuery.exec()) {
                    db.rollback();
                    qDebug() << "Ошибка сохранения:" << updateQuery.lastError().text();
                    qDebug() << "Failed query:" << updateQuery.lastQuery();
                    return tmp;
                }
            }
            db.commit();
            qDebug() << "Результаты успешно сохранены в столбец" << saveColumn;
        } catch (...) {
            db.rollback();
            qDebug() << "Ошибка при сохранении результатов";
        }
    }

    return tmp;
}

#endif // CEPSTRAL_H
