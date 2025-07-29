#ifndef CORREL_H
#define CORREL_H

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

solv_struct avcorrel(double size)
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

        // **Получаем minId из базы данных**
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
            return solv_struct();  // Или установите minId = 0; если это приемлемо
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
        Ex.clear(); // Очищаем Ex перед заполнением
        Ex.reserve(size); // Зарезервируем место заранее

        for (int i = 0; i < size; ++i) {
            QSqlQuery query(db);
            query.prepare("SELECT " + columnName + " FROM " + tableName + " WHERE id = ?");
            query.addBindValue(minId + i);

            if (!query.exec()) {
                qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
                Ex.append(0.0); // Обрабатываем ошибку, добавив значение по умолчанию
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
                    Ex.append(0.0); // Обрабатываем ошибку преобразования
                }
            } else {
                qDebug() << "Нет данных для id = " << minId + i;
                Ex.append(0.0); // Если нет данных по этому ID, добавляем 0.0 (или другое значение по умолчанию)
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

    int limit = size;
    double mean = 0.0;
    for (int i = 0; i < Ex.size(); i++) {
        mean += Ex.at(i);
        qDebug() << Ex[i];
    }
    mean /= Ex.size();

    std::vector<double> deltaX(limit);
    for (int i = 0; i < limit; ++i) {
        deltaX[i] = Ex[i] - mean;
    }

    std::vector<double> Rxx(limit + 1);
    float var = 0.0;
    for (int i = 0; i < Ex.size(); i++) {
        var += (Ex[i] - mean) * (Ex[i] - mean);
    }
    var /= Ex.size();
    Rxx[0] = 1;

    for (int lag = 1; lag <= limit; lag++) {
        double c = 0.0;
        for (int j = 0; j < limit - lag; j++) {
            c += deltaX[j + lag] * deltaX[j];
        }
        c /= (limit * var);
        Rxx[lag] = c;
    }

    solv_struct tmp;
    tmp.veccor1 = Rxx;

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

            for (int i = 0; i < Rxx.size(); ++i) {
                updateQuery.bindValue(":value", Rxx[i]);
                updateQuery.bindValue(":id", minId + i);

                qDebug() << "Setting value" << Rxx[i] << "for id" << (minId + i);

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
#endif // CORREL_H
