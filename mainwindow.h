#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QChartView>
#include <QTableWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QtSql>
#include <QDebug>

#include <QSqlDatabase>
#include "databaseManager.h"


    class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void createButtons(QHBoxLayout* buttonsLayout);
    void createTabs();
    void createEpsInput(QHBoxLayout* inputLayout);
    void createRangeControl();
    void updateSlidersRange();
    void applyRange();
    void loadMeasurementsTable();
    bool initializeDatabase();


    QVBoxLayout* mainLayout;
    QTabWidget* tabWidget;

    // Кнопки для вычислений
    QPushButton* seriesButton;
    QPushButton* inversionsButton;
    QPushButton* autocorrelationButton;
    QPushButton* crossCorrelationButton;
    QPushButton* spectralAnalysisButton;
    QPushButton* cepstralAnalysisButton;

    // График
    QChartView* chartView;

    // Таблица для значений
    QTableWidget* dataTable;

    QLineEdit* epsLineEdit; // Поле ввода для eps
    QLabel* epsLabel;  // Метка для поля ввода eps

    QLineEdit* divisionLineEdit;
    QLabel* divisionLabel;

       // Слайдер для управления диапазоном
    QWidget* rangeControlContainer;
    QSlider* minSlider;
    QSlider* maxSlider;
    QLineEdit* minLineEdit;
    QLineEdit* maxLineEdit;

    DatabaseManager* dbManager;

public slots:
    void setXAxisRange(int min, int max);
    void onSeriesButtonClicked();
    void onSpectralAnalysisButtonClicked();
    void onAutocorrelationButtonClicked();
    void onCrossCorrelationButtonClicked();
    void onInversionsButtonButtonClicked();
    void onCepstralAnalysisButtonClicked();
};
#endif // MAINWINDOW_H
