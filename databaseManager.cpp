#include "databaseManager.h"

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool DatabaseManager::connectToDatabase(const QString &host, const QString &dbName,
                                        const QString &user, const QString &password)
{
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(host);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);

    if (!db.open()) {
        qDebug() << "Error: connection with database failed";
        qDebug() << db.lastError().text();
        return false;
    }

    qDebug() << "Database: connection ok";
    return true;
}

QSqlQuery DatabaseManager::executeQuery(const QString &query)
{
    QSqlQuery sqlQuery(db);
    if (!sqlQuery.exec(query)) {
        qDebug() << "Query failed:" << sqlQuery.lastError().text();
        qDebug() << "Query text:" << query;
    }
    return sqlQuery;
}

bool DatabaseManager::isConnected() const
{
    return db.isOpen();
}

QList<QList<QVariant>> DatabaseManager::getTableData(const QString &tableName)
{
    QList<QList<QVariant>> result;

    if (!isConnected()) {
        qDebug() << "No database connection";
        return result;
    }

    QString queryString = QString("SELECT * FROM %1").arg(tableName);
    QSqlQuery query = executeQuery(queryString);

    while (query.next()) {
        QList<QVariant> row;
        for (int i = 0; i < query.record().count(); ++i) {
            row.append(query.value(i));
        }
        result.append(row);
    }

    return result;
}

QStringList DatabaseManager::getTableColumns(const QString &tableName)
{
    QStringList columns;

    if (!isConnected()) {
        qDebug() << "No database connection";
        return columns;
    }

    QString queryString = QString("SELECT * FROM %1 LIMIT 0").arg(tableName);
    QSqlQuery query = executeQuery(queryString);

    QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); ++i) {
        columns.append(record.fieldName(i));
    }

    return columns;
}
