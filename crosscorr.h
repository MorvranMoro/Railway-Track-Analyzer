#ifndef CROSSCORR_H
#define CROSSCORR_H

#pragma once

#include <vector>
#include <numeric>
#include <algorithm>
#include "struct.h"
#include <windows.h>
#include <commdlg.h>
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

solv_struct crosscorr(double size) {
    // Диалог выбора источника данных
    QMessageBox msgBox;
    msgBox.setText("Выберите источник данных");
    QPushButton *fileButton = msgBox.addButton("Из файлов", QMessageBox::ActionRole);
    QPushButton *dbButton = msgBox.addButton("Из базы данных", QMessageBox::ActionRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.exec();

    QVector<double> Ex, Ey;
    QString tableName1, tableName2;
    QString columnName1, columnName2;
    int minId = -1;

    if (msgBox.clickedButton() == fileButton) {
        // Загрузка первого файла
        QString filePath = QFileDialog::getOpenFileName(nullptr, "Выберите первый файл", "", "Text Files (*.txt)");
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Не удалось открыть первый файл для чтения";
            return {};
        }
        QTextStream in(&file);
        while (!in.atEnd() && Ex.size() < size) {
            double value;
            in >> value;
            Ex.append(value);
        }
        file.close();

        // Загрузка второго файла
        QString filePath1 = QFileDialog::getOpenFileName(nullptr, "Выберите второй файл", "", "Text Files (*.txt)");
        QFile file1(filePath1);
        if (!file1.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Не удалось открыть второй файл для чтения";
            return {};
        }
        QTextStream in1(&file1);
        while (!in1.atEnd() && Ey.size() < size) {
            double value;
            in1 >> value;
            Ey.append(value);
        }
        file1.close();
    }
    else if (msgBox.clickedButton() == dbButton) {
        // Загрузка из базы данных
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            qDebug() << "Нет подключения к базе данных";
            return {};
        }

        // Выбор первой таблицы и столбца
        QStringList tables = db.tables(QSql::Tables);
        tableName1 = QInputDialog::getItem(nullptr, "Выбор первой таблицы",
                                           "Выберите таблицу для первого набора данных:", tables, 0, false);
        if (tableName1.isEmpty()) return {};

        // Получаем minId из первой таблицы
        QSqlQuery idQuery(db);
        if (!idQuery.exec("SELECT MIN(id) FROM " + tableName1)) {
            qDebug() << "Ошибка получения minId:" << idQuery.lastError().text();
            return {};
        }
        if (idQuery.next()) {
            minId = idQuery.value(0).toInt();
        }

        // Получаем столбцы первой таблицы
        QSqlQuery columnQuery(db);
        if (!columnQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = '" +
                              tableName1 + "'")) {
            qDebug() << "Ошибка получения столбцов:" << columnQuery.lastError().text();
            return {};
        }
        QStringList columns1;
        while (columnQuery.next()) {
            columns1 << columnQuery.value(0).toString();
        }
        columnName1 = QInputDialog::getItem(nullptr, "Выбор столбца",
                                            "Выберите первый столбец с данными:", columns1, 0, false);
        if (columnName1.isEmpty()) return {};

        // Выбор второй таблицы и столбца
        tableName2 = QInputDialog::getItem(nullptr, "Выбор второй таблицы",
                                           "Выберите таблицу для второго набора данных:", tables, 0, false);
        if (tableName2.isEmpty()) return {};

        // Получаем столбцы второй таблицы
        if (!columnQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = '" +
                              tableName2 + "'")) {
            qDebug() << "Ошибка получения столбцов:" << columnQuery.lastError().text();
            return {};
        }
        QStringList columns2;
        while (columnQuery.next()) {
            columns2 << columnQuery.value(0).toString();
        }
        columnName2 = QInputDialog::getItem(nullptr, "Выбор столбца",
                                            "Выберите второй столбец с данными:", columns2, 0, false);
        if (columnName2.isEmpty()) return {};

        // Загрузка данных из первого столбца
        Ex.reserve(size);
        for (int i = 0; i < size; ++i) {
            QSqlQuery query(db);
            query.prepare("SELECT " + columnName1 + " FROM " + tableName1 + " WHERE id = ?");
            query.addBindValue(minId + i);
            if (!query.exec() || !query.next()) {
                Ex.append(0.0);
                continue;
            }
            bool ok;
            double value = query.value(0).toDouble(&ok);
            Ex.append(ok ? value : 0.0);
        }

        // Загрузка данных из второго столбца
        Ey.reserve(size);
        for (int i = 0; i < size; ++i) {
            QSqlQuery query(db);
            query.prepare("SELECT " + columnName2 + " FROM " + tableName2 + " WHERE id = ?");
            query.addBindValue(minId + i);
            if (!query.exec() || !query.next()) {
                Ey.append(0.0);
                continue;
            }
            bool ok;
            double value = query.value(0).toDouble(&ok);
            Ey.append(ok ? value : 0.0);
        }
    }
    else {
        return {};
    }

    // Проверка данных
    if (Ex.isEmpty() || Ey.isEmpty() || Ex.size() != Ey.size()) {
        qDebug() << "Ошибка: некорректные данные для анализа";
        return {};
    }

    // Вычисление средних значений
    double meanEx = accumulate(Ex.begin(), Ex.end(), 0.0) / Ex.size();
    double meanEy = accumulate(Ey.begin(), Ey.end(), 0.0) / Ey.size();

    // Вычисление дисперсий
    double varEx = 0, varEy = 0;
    for (int i = 0; i < Ex.size(); ++i) {
        varEx += pow(Ex[i] - meanEx, 2);
        varEy += pow(Ey[i] - meanEy, 2);
    }
    varEx = sqrt(varEx / Ex.size());
    varEy = sqrt(varEy / Ey.size());

    // Вычисление кросс-корреляции
    int limit = size;
    vector<double> Rxyx(limit), Rxyy(limit);

    for (int lag = 0; lag < limit; lag++) {
        double sum1 = 0, sum2 = 0;
        int count = limit - lag;

        for (int j = 0; j < count; j++) {
            sum1 += (Ey[j + lag] - meanEy) * (Ex[j] - meanEx);
            sum2 += (Ex[j + lag] - meanEx) * (Ey[j] - meanEy);
        }

        Rxyy[lag] = sum1 / (count * varEx * varEy);
        Rxyx[lag] = sum2 / (count * varEx * varEy);
    }

    solv_struct result;
    result.veccor1 = Rxyx;
    result.veccor2 = Rxyy;

    // Сохранение результатов в БД
    QMessageBox saveMsgBox;
    saveMsgBox.setText("Сохранить результаты в базу данных?");
    saveMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (saveMsgBox.exec() == QMessageBox::Yes) {
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            qDebug() << "Нет подключения к базе данных";
            return result;
        }

        // Выбор таблицы для сохранения
        QStringList tables = db.tables(QSql::Tables);
        QString saveTable = QInputDialog::getItem(nullptr, "Выбор таблицы",
                                                  "Выберите таблицу для сохранения:", tables, 0, false);
        if (saveTable.isEmpty()) return result;

        // Выбор столбца для сохранения
        QSqlQuery columnQuery(db);
        if (!columnQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = '" +
                              saveTable + "'")) {
            qDebug() << "Ошибка получения столбцов:" << columnQuery.lastError().text();
            return result;
        }
        QStringList columns;
        while (columnQuery.next()) {
            columns << columnQuery.value(0).toString();
        }
        QString saveColumn = QInputDialog::getItem(nullptr, "Выбор столбца",
                                                   "Выберите столбец для сохранения результатов:",
                                                   columns, 0, false);
        if (saveColumn.isEmpty()) return result;

        // Объединение результатов в один вектор
        vector<double> combinedResults;
        combinedResults.reserve(Rxyx.size() + Rxyy.size());
        combinedResults.insert(combinedResults.end(), Rxyx.begin(), Rxyx.end());
        combinedResults.insert(combinedResults.end(), Rxyy.begin(), Rxyy.end());

        // Сохранение объединенных результатов
        // Сохранение объединенных результатов
        db.transaction();
        try {
            QSqlQuery updateQuery(db);
            QString queryText = QString("UPDATE \"%1\" SET \"%2\" = ? WHERE id = ?")
                                    .arg(saveTable, saveColumn);

            if (!updateQuery.prepare(queryText)) {
                qDebug() << "Ошибка подготовки запроса:" << updateQuery.lastError().text();
                db.rollback();
                return result;
            }

            for (size_t i = 0; i < combinedResults.size(); ++i) {
                updateQuery.addBindValue(combinedResults[i]);
                updateQuery.addBindValue(minId + static_cast<int>(i));

                if (!updateQuery.exec()) {
                    qDebug() << "Ошибка выполнения запроса:" << updateQuery.lastError().text();
                    qDebug() << "Запрос:" << updateQuery.lastQuery();
                    db.rollback();
                    return result;
                }
            }
            db.commit();
            qDebug() << "Результаты успешно сохранены в столбец" << saveColumn;
        }
        catch (const QSqlError &e) {
            db.rollback();
            qDebug() << "Ошибка SQL:" << e.text();
        }
        catch (...) {
            db.rollback();
            qDebug() << "Неизвестная ошибка при сохранении результатов";
        }
    }

    return result;
}

#endif // CROSSCORR_H
