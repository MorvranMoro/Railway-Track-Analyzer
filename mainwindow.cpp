#include "mainwindow.h"
#include <QDebug>
#include <QChart>
#include <QLineSeries>
#include <QBarSet>
#include <QBarSeries>
#include <QValueAxis>
#include <vector>
#include "specanal.h"
#include "correl.h"
#include "serials.h"
#include "inversions.h"
#include "crosscorr.h"
#include "Cepstral.h"
#include "struct.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QtMath>
#include <QSlider>
#include <QtSql>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Настройка главного окна
    setWindowTitle("Data Analysis Tool");
    resize(1200, 800); //увеличиваем размер для большего места

    QWidget* centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    // Главный layout
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Создаем контейнер для кнопок
    QWidget *buttonsContainer = new QWidget();
    QHBoxLayout *buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setSpacing(5);  // Уменьшаем отступы между кнопками

    createButtons(buttonsLayout);
    mainLayout->addWidget(buttonsContainer);

    // Создаем контейнер для ввода
    QWidget *inputContainer = new QWidget();
    QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
    createEpsInput(inputLayout);
    mainLayout->addWidget(inputContainer);

    createTabs();
    createRangeControl(); // Создаем панель управления диапазоном
    mainLayout->addWidget(rangeControlContainer);
    rangeControlContainer->hide();

    centralWidget->setStyleSheet(
        "QWidget { background-color: #f0f0f0; }"
        "QPushButton { background-color: #e0e0e0; border: none; padding: 10px; margin: 2px; border-radius: 5px; }"
        "QPushButton:hover { background-color: #d0d0d0; }"
        "QTabWidget::pane { border-top: 2px solid #c0c0c0; }"
        "QTabBar::tab { background: #e0e0e0; border: none; padding: 8px 20px;  border-top-left-radius: 5px; border-top-right-radius: 5px;}"
        "QTabBar::tab:selected { background: #f0f0f0; border-bottom: 2px solid #007bff;}" // С подчеркиванием
        "QTableWidget { background-color: white; }"
        "QHeaderView::section { background-color: #e0e0e0; border: none; border-bottom: 1px solid #c0c0c0; }"
        "QHeaderView::section:horizontal{ border-bottom: none;}"
        "QHeaderView::section:vertical{border-right: none;}"
        "QLabel { font-size: 12px; color: #555; }" //стиль для QLabel
        "QLineEdit { padding: 8px; border: 1px solid #c0c0c0; border-radius: 3px; }"
        );


    // Связывание кнопки "Спектральный анализ" со слотом
    connect(spectralAnalysisButton, &QPushButton::clicked, this, &MainWindow::onSpectralAnalysisButtonClicked);
    // Связывание кнопки "Критерий серий" со слотом
    connect(seriesButton, &QPushButton::clicked, this, &MainWindow::onSeriesButtonClicked);
    // Связывание кнопки "Автокорреляция" со слотом
    connect(autocorrelationButton, &QPushButton::clicked, this, &MainWindow::onAutocorrelationButtonClicked);
    // Связывание кнопки "Взаимная корреляция" со слотом
    connect(crossCorrelationButton, &QPushButton::clicked, this, &MainWindow::onCrossCorrelationButtonClicked);
    // Связывание кнопки "Критерий инверсий" со слотом
    connect(inversionsButton, &QPushButton::clicked, this, &MainWindow::onInversionsButtonButtonClicked);
    connect(cepstralAnalysisButton, &QPushButton::clicked, this, &MainWindow::onCepstralAnalysisButtonClicked);



    dbManager = new DatabaseManager(this);
    if (!initializeDatabase()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных");
    } else {
        loadMeasurementsTable();  // Загрузка данных при успешном подключении
    }




}


MainWindow::~MainWindow() {
    delete dbManager;
}


bool MainWindow::initializeDatabase()
{
    // Здесь укажите свои параметры подключения
    return dbManager->connectToDatabase(
        "localhost",  // хост
        "postgres",      // имя базы данных
        "postgres",   // пользователь
        "030303" // пароль
        );
}

void MainWindow::loadMeasurementsTable()
{
    if (!dbManager->isConnected()) {
        QMessageBox::warning(this, "Ошибка", "Нет подключения к базе данных");
        return;
    }

    // Получаем данные из таблицы measurements
    QStringList columns = dbManager->getTableColumns("measurements");
    QList<QList<QVariant>> data = dbManager->getTableData("measurements");

    // Настраиваем таблицу
    dataTable->clear();
    dataTable->setColumnCount(columns.size());
    dataTable->setHorizontalHeaderLabels(columns);
    dataTable->setRowCount(data.size());

    // Заполняем таблицу данными
    for (int row = 0; row < data.size(); ++row) {
        for (int col = 0; col < data[row].size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(data[row][col].toString());
            dataTable->setItem(row, col, item);
        }
    }

    // Автоматическое выравнивание столбцов
    dataTable->resizeColumnsToContents();
}

void MainWindow::createButtons(QHBoxLayout* buttonsLayout) {
    QSize iconSize(24,24);

    // Создание кнопок с иконками
    seriesButton = new QPushButton("Критерий серий");
    inversionsButton = new QPushButton("Критерий инверсий");
    autocorrelationButton = new QPushButton("Автокорреляция");
    crossCorrelationButton = new QPushButton("Взаимная корреляция");
    spectralAnalysisButton = new QPushButton("Спектральный анализ");
    cepstralAnalysisButton = new QPushButton("Кепстральный анализ");

    seriesButton->setToolTip("Критерий серий");
    inversionsButton->setToolTip("Критерий инверсий");
    autocorrelationButton->setToolTip("Автокорреляция");
    crossCorrelationButton->setToolTip("Взаимная корреляция");
    spectralAnalysisButton->setToolTip("Спектральный анализ");
    cepstralAnalysisButton->setToolTip("Кепстральный анализ");

    // Установка размера иконки
    seriesButton->setIconSize(iconSize);
    inversionsButton->setIconSize(iconSize);
    autocorrelationButton->setIconSize(iconSize);
    crossCorrelationButton->setIconSize(iconSize);
    spectralAnalysisButton->setIconSize(iconSize);
    cepstralAnalysisButton->setIconSize(iconSize);

    // Добавление кнопок на layout
    buttonsLayout->addWidget(seriesButton);
    buttonsLayout->addWidget(inversionsButton);
    buttonsLayout->addWidget(autocorrelationButton);
    buttonsLayout->addWidget(crossCorrelationButton);
    buttonsLayout->addWidget(spectralAnalysisButton);
    buttonsLayout->addWidget(cepstralAnalysisButton);

    QPushButton* refreshDataButton = new QPushButton("Обновить данные");
    refreshDataButton->setIcon(QIcon(":/icons/refresh.png"));
    refreshDataButton->setToolTip("Обновить данные из БД");
    buttonsLayout->addWidget(refreshDataButton);
    connect(refreshDataButton, &QPushButton::clicked, this, &MainWindow::loadMeasurementsTable);

}


void MainWindow::createTabs() {
    // Создание вкладок
    tabWidget = new QTabWidget();

    // Вкладка для графика
    chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);
    tabWidget->addTab(chartView, "График");

    // Вкладка для таблицы значений
    dataTable = new QTableWidget();
    dataTable->setColumnCount(4);
    dataTable->setHorizontalHeaderLabels({"X", "Y", "Value1","Value2"});
    tabWidget->addTab(dataTable, "Значения");

    // Добавление вкладок на главный layout
    mainLayout->addWidget(tabWidget);
}

void MainWindow::createEpsInput(QHBoxLayout* inputLayout){
    epsLabel = new QLabel("Epsilon:", this);
    epsLineEdit = new QLineEdit(this);
    epsLineEdit->setText("2048");

    divisionLabel = new QLabel("Division:", this);
    divisionLineEdit = new QLineEdit(this);
    divisionLineEdit->setText("12");


    // Размещаем в одной строке
    inputLayout->addWidget(epsLabel);
    inputLayout->addWidget(epsLineEdit);
    inputLayout->addWidget(divisionLabel);
    inputLayout->addWidget(divisionLineEdit);

    // Добавляем стили для более современного вида
    epsLineEdit->setStyleSheet("border: 1px solid #c0c0c0; border-radius: 3px;");
    divisionLineEdit->setStyleSheet("border: 1px solid #c0c0c0; border-radius: 3px;");

}

void MainWindow::createRangeControl() {
    // Создаем панель для элементов управления диапазоном оси X
    rangeControlContainer = new QWidget();
    QHBoxLayout *rangeControlLayout = new QHBoxLayout(rangeControlContainer);
    rangeControlLayout->setSpacing(10);


    QLabel *minLabel = new QLabel("Min:", this);
    minSlider = new QSlider(Qt::Horizontal, this);
    minSlider->setMinimum(-100);
    minSlider->setMaximum(100);
    minSlider->setValue(0);

    minLineEdit = new QLineEdit(this);
    minLineEdit->setText(QString::number(-10000)); //начальное значение
    connect(minLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateSlidersRange);


    QLabel *maxLabel = new QLabel("Max:", this);
    maxSlider = new QSlider(Qt::Horizontal, this);
    maxSlider->setMinimum(-100);
    maxSlider->setMaximum(100);
    maxSlider->setValue(100);

    maxLineEdit = new QLineEdit(this);
    maxLineEdit->setText(QString::number(10000));//начальное значение
    connect(maxLineEdit, &QLineEdit::editingFinished, this, &MainWindow::updateSlidersRange);


    // Создаем кнопку "Применить"
    QPushButton *applyRangeButton = new QPushButton("Применить", this);
    connect(applyRangeButton, &QPushButton::clicked, this, &MainWindow::applyRange);

    // Настраиваем размеры
    minSlider->setMinimumWidth(150);
    maxSlider->setMinimumWidth(150);


    // Добавляем элементы на layout
    rangeControlLayout->addWidget(minLabel);
    rangeControlLayout->addWidget(minSlider);
    rangeControlLayout->addWidget(minLineEdit);
    rangeControlLayout->addWidget(maxLabel);
    rangeControlLayout->addWidget(maxSlider);
    rangeControlLayout->addWidget(maxLineEdit);
    rangeControlLayout->addWidget(applyRangeButton);


    // Общие стили
    rangeControlContainer->setStyleSheet("QWidget { background-color: #f0f0f0; padding: 5px;}");
    applyRangeButton->setStyleSheet("QPushButton {padding: 5px; margin: 5px; }");
}

void MainWindow::updateSlidersRange()
{
    bool ok_min, ok_max;
    int minVal = minLineEdit->text().toInt(&ok_min);
    int maxVal = maxLineEdit->text().toInt(&ok_max);

    if (ok_min && ok_max && minVal < maxVal) {
        minSlider->setMinimum(minVal);
        minSlider->setMaximum(maxVal);
        maxSlider->setMinimum(minVal);
        maxSlider->setMaximum(maxVal);

        if (minSlider->value() < minVal)
            minSlider->setValue(minVal);
        if (maxSlider->value() > maxVal)
            maxSlider->setValue(maxVal);

    }
}

void MainWindow::applyRange()
{
    setXAxisRange(minSlider->value(), maxSlider->value());
}

void MainWindow::setXAxisRange(int min, int max) {

        // Получаем указатель на QChart из QChartView
    QChart *chart = chartView->chart();
    if(!chart) return;

    //Проверка на валидность min и max
    if (min >= max ) {
        QMessageBox::warning(this, "Ошибка", "Некорректный диапазон осей X");
        return;
    }
    // Проверка, что у нас вообще есть ось X
    if (chart->axes(Qt::Horizontal).isEmpty()){
        return;
    }
    // Устанавливаем диапазон оси X
    chart->axes(Qt::Horizontal).first()->setRange(min, max);
}

void MainWindow::onSeriesButtonClicked(){
    // 1. Запрос eps
    bool ok_eps, ok_division;
    double eps = epsLineEdit->text().toDouble(&ok_eps);
    int division = divisionLineEdit->text().toInt(&ok_division);
    if(!ok_eps || !ok_division)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать введенное значение в число");
        return;
    }

    // 2. Получение данных
    solv_struct tmp = serialss(eps, division);
    std::vector<double> returnedUQr = tmp.Qr;
    std::vector<double> returnedUr = tmp.Ur;
    //double Bob = tmp.Bobby;
   // double Bob1 = tmp.Bobby2;


    // 3. Создание графика и добавление данных
    QChart *chart = new QChart();
    chart->setTitle("Критерий серий");
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < returnedUQr.size(); i++) {
        series->append(i, returnedUQr[i]);
    }
    QLineSeries *seriess = new QLineSeries();
    for (int i = 0; i < returnedUr.size(); i++)
    {
        seriess->append(i, returnedUr[i]);
    }

    chart->addSeries(series);
    chart->addSeries(seriess);
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText("Количество секторов");
    chart->axes(Qt::Vertical).first()->setTitleText("Числовые значения");
    seriess->setName("Среднее значение");
    series->setName("Среднеквадратическое отклонение");
        // Установка графика в chartView
    chartView->setChart(chart);
    chartView->chart()->setAnimationOptions(QChart::AllAnimations);

    // 4. Заполнение таблицы данными
    dataTable->clearContents();
    int size = qMax(returnedUQr.size(), returnedUr.size());
    dataTable->setRowCount(size);

    for (int row = 0; row < size; ++row) {
        dataTable->setItem(row, 0, new QTableWidgetItem(QString::number(row+1)));
        dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(returnedUQr[row])));
        dataTable->setItem(row, 3, new QTableWidgetItem(QString::number(returnedUr[row])));
    }
    //5. Настройка элементов для управления диапазоном оси Х
    if (!rangeControlContainer->isVisible()) {
        rangeControlContainer->show();
    }

}

void MainWindow::onInversionsButtonButtonClicked(){
    // 1. Запрос eps
    bool ok_eps, ok_division;
    double eps = epsLineEdit->text().toDouble(&ok_eps);
    int division = divisionLineEdit->text().toInt(&ok_division);
    if(!ok_eps || !ok_division)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать введенное значение в число");
        return;
    }

    // 2. Получение данных
    solv_struct tmp = inversions(eps, division);
    std::vector<double> returnedUQr = tmp.Qr;
    std::vector<double> returnedUr = tmp.Ur;
    double Bob = tmp.Bobby;
    double Bob1 = tmp.Bobby2;


    // 3. Создание графика и добавление данных
    QChart *chart = new QChart();
    chart->setTitle("Критерий серий");
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < returnedUQr.size(); i++) {
        series->append(i, returnedUQr[i]);
    }
    QLineSeries *seriess = new QLineSeries();
    for (int i = 0; i < returnedUr.size(); i++)
    {
        seriess->append(i, returnedUr[i]);
    }

    chart->addSeries(series);
    chart->addSeries(seriess);
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText("Количество секторов");
    chart->axes(Qt::Vertical).first()->setTitleText("Числовые значения");
    seriess->setName("Среднее значение");
    series->setName("Среднеквадратическое отклонение");
        // Установка графика в chartView
    chartView->setChart(chart);
    chartView->chart()->setAnimationOptions(QChart::AllAnimations);

    // 4. Заполнение таблицы данными
    dataTable->clearContents();
    int size = qMax(returnedUQr.size(), returnedUr.size());
    dataTable->setRowCount(size);

    for (int row = 0; row < size; ++row) {
        dataTable->setItem(row, 0, new QTableWidgetItem(QString::number(row+1)));
        dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(returnedUQr[row])));
        dataTable->setItem(row, 3, new QTableWidgetItem(QString::number(returnedUr[row])));
    }
    //5. Настройка элементов для управления диапазоном оси Х
    if (!rangeControlContainer->isVisible()) {
        rangeControlContainer->show();
    }
}

void MainWindow::onSpectralAnalysisButtonClicked(){
    // 1. Получение eps
    bool ok;
    double eps = epsLineEdit->text().toDouble(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать введенное значение в число");
        return;
    }
    // 2. Получение данных
    solv_struct tmp = getsolve(eps);
    std::vector<double> ser = tmp.vec;

    // 3. Создание графика и добавление данных
    QChart *chart = new QChart();
    chart->setTitle("Спектральный анализ");
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < ser.size(); i++) {
        series->append(i, ser[i]);
    }

    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText("Частота");
    chart->axes(Qt::Vertical).first()->setTitleText("Мощность");

    // Установка графика в chartView
    chartView->setChart(chart);
    chartView->chart()->setAnimationOptions(QChart::AllAnimations);


    // 4. Заполнение таблицы данными
    dataTable->clearContents();
    dataTable->setRowCount(ser.size());
    for (int row = 0; row < ser.size(); ++row) {
        dataTable->setItem(row, 0, new QTableWidgetItem(QString::number(row)));
        dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(0)));
        dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(ser[row])));
    }

    if (!rangeControlContainer->isVisible()) {
        rangeControlContainer->show();
    }
}

void MainWindow::onCepstralAnalysisButtonClicked(){
    // 1. Получение eps
    bool ok;
    double eps = epsLineEdit->text().toDouble(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать введенное значение в число");
        return;
    }
    // 2. Получение данных
    solv_struct tmp = ceps(eps);
    std::vector<double> ser = tmp.veccor1;

    // 3. Создание графика и добавление данных
    QChart *chart = new QChart();
    chart->setTitle("Кепстральный анализ");
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < ser.size(); i++) {
        series->append(i, ser[i]);
    }

    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setTitleText("Частота");
    chart->axes(Qt::Vertical).first()->setTitleText("Мощность");

    // Установка графика в chartView
    chartView->setChart(chart);
    chartView->chart()->setAnimationOptions(QChart::AllAnimations);


    // 4. Заполнение таблицы данными
    dataTable->clearContents();
    dataTable->setRowCount(ser.size());
    for (int row = 0; row < ser.size(); ++row) {
        dataTable->setItem(row, 0, new QTableWidgetItem(QString::number(row)));
        dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(0)));
        dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(ser[row])));
    }

    if (!rangeControlContainer->isVisible()) {
        rangeControlContainer->show();
    }

}


void MainWindow::onAutocorrelationButtonClicked()
{
    // 1. Получение eps
    bool ok;
    double eps = epsLineEdit->text().toDouble(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать введенное значение в число");
        return;
    }

    // 2. Получение данных
    solv_struct tmp = avcorrel(eps);
    std::vector<double> ser = tmp.veccor1;

    // 3. Подготовка нового графика
    QChart *chart = new QChart();
    chart->setTitle("Автокорреляция");

    // 4. Создание и заполнение серии данных
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < ser.size(); i++) {
        series->append(i, ser[i]);
    }
    chart->addSeries(series);

    // 5. Создание осей
    chart->createDefaultAxes();

    // 6. Настройка осей
    QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());

    if (axisX) {
        axisX->setTitleText("Запаздывание");
    }

    if (axisY) {
        axisY->setTitleText("Коэффициент корреляции");
        axisY->setRange(-1.0, 1.0);
        axisY->setTickCount(11);
    }

    // 7. Безопасная замена графика
    QChart *oldChart = chartView->chart();
    chartView->setChart(chart);
    if (oldChart) {
        oldChart->deleteLater(); // Безопасное удаление старого графика
    }

    // 8. Настройка анимации
    chartView->chart()->setAnimationOptions(QChart::AllAnimations);

    // 9. Заполнение таблицы
    dataTable->clearContents();
    dataTable->setRowCount(ser.size());
    for (int row = 0; row < ser.size(); ++row) {
        dataTable->setItem(row, 0, new QTableWidgetItem(QString::number(row)));
        dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(0)));
        dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(ser[row])));
    }

    // 10. Показ элементов управления
    if (!rangeControlContainer->isVisible()) {
        rangeControlContainer->show();
    }
}

void MainWindow::onCrossCorrelationButtonClicked()
{
    // 1. Получение eps
    bool ok;
    double eps = epsLineEdit->text().toDouble(&ok);
    if(!ok)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать введенное значение в число");
        return;
    }

    // 2. Получение данных
    solv_struct tmp = crosscorr(eps);
    std::vector<double> returnedVector1 = tmp.veccor1;
    std::vector<double> returnedVector2 = tmp.veccor2;

    // 3. Подготовка нового графика
    QChart *chart = new QChart();
    chart->setTitle("Взаимная корреляция");

    // 4. Создание и заполнение серий данных
    QLineSeries *series = new QLineSeries();
    QLineSeries *seriess = new QLineSeries();

    int k = 0;
    int kk = 0;
    for (const auto& num : returnedVector1) {
        *series << QPointF(k, num);
        ++k;
    }
    for (const auto& numm : returnedVector2) {
        *seriess << QPointF(-kk, numm);
        ++kk;
    }

    // 5. Добавление серий на график
    chart->addSeries(series);
    chart->addSeries(seriess);
    chart->createDefaultAxes();

    // 6. Настройка осей
    QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());

    if (axisX) {
        axisX->setTitleText("Запаздывание");
        axisX->setLabelFormat("%d");
        axisX->setTickCount(10);
    }

    if (axisY) {
        axisY->setTitleText("Коэффициент корреляции");
        axisY->setRange(-1.0, 1.0);  // Фиксированный диапазон Y от -1 до 1
        axisY->setTickCount(11);     // Деления через каждые 0.2
    }

    // 7. Безопасная замена графика
    QChart *oldChart = chartView->chart();
    chartView->setChart(chart);
    if (oldChart) {
        oldChart->deleteLater(); // Безопасное удаление старого графика
    }

    // 8. Настройка анимации
    chartView->chart()->setAnimationOptions(QChart::AllAnimations);

    // 9. Заполнение таблицы
    dataTable->clearContents();
    int size = qMax(returnedVector1.size(), returnedVector2.size());
    dataTable->setRowCount(size);
    for (int row = 0; row < size; ++row) {
        dataTable->setItem(row, 0, new QTableWidgetItem(QString::number(row)));
        dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(0)));
        if(row < returnedVector1.size()) {
            dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(returnedVector1[row])));
        }
        if(row < returnedVector2.size()){
            dataTable->setItem(row, 3, new QTableWidgetItem(QString::number(returnedVector2[row])));
        }
    }

    // 10. Показ элементов управления
    if (!rangeControlContainer->isVisible()) {
        rangeControlContainer->show();
    }
}
