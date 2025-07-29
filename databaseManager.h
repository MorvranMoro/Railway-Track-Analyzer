#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QtSql>
#include <QDebug>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool connectToDatabase(const QString &host, const QString &dbName,
                           const QString &user, const QString &password);
    QSqlQuery executeQuery(const QString &query);
    bool isConnected() const;

    QList<QList<QVariant>> getTableData(const QString &tableName);
    QStringList getTableColumns(const QString &tableName);

private:
    QSqlDatabase db;
};

#endif // DATABASEMANAGER_H
