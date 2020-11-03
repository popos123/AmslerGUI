#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QList>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QTimer>
#include <QFileDevice>
#include <QMessageBox>

int ena = 0;
int interval = 100;
int old_interval = 0;
int Analog_ = 0;
int Digital_ = 0;
int cal_dig = 0;
int cal_anl = 80;
float Analog_c = 0.0;
int Digital_c = 0;
float gain_c = 10000.0;
float max_analog = 32767.0;
QString file_name = "";
double last_time = 0;
double key = 0;
QTime* timer2 = new QTime();
bool first_save_file = 1;
bool scroll_time = 0, disable_tag_1 = 0,disable_tag_2 = 0, disable_tag_3 = 0;
uint64_t time_int = 10;
bool auto_disable = 1;
bool old_ena = 0;
uint64_t rec_cnt = 0, send_cnt = 0;
bool rd = 0;
bool ena_timer = 1;
double r = (40.0*0.001)/2.0, rpm = 400.0, v, distance;
bool rec = 0;
double force = 0, mass = 1, u = 0;
double last_reset = 0;
bool set_default_gain = 0;
bool checked_test = 0;

float map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Amsler GUI");
    this->device = new QSerialPort(this);
    on_pushButtonSearch_clicked(); // for fill a combo box
    autoConnect();
    makePlot();
    on_checkBox_8_toggled(1);
    ui->pushButtonLedOff->setDisabled(true);
    ui->pushButtonLedOff_2->setDisabled(true);
    // start of disable a force graph =================================================
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2)->setVisible(false); // hide axis
    mGraph3->setVisible(false); // hide graph
    mTag1->updatePosition(10000); // move out tag
    mTag2->updatePosition(10000); // move out tag
    mTag3->updatePosition(10000); // move out tag
    //ena = 1;
    timerSlot(); // for disable legend
    ui->centralWidget_2->replot();
    // end of disable a force graph ===================================================
    // register the plot document object (only needed once, no matter how many plots will be in the QTextDocument):
    QCPDocumentObject *plotObjectHandler = new QCPDocumentObject(this);
    ui->textBrowser->document()->documentLayout()->registerHandler(QCPDocumentObject::PlotTextFormat, plotObjectHandler);
    // fill and select number of weight
    QStringList range = {"brak", "1", "2", "3"};
    ui->comboBoxDevices_3->addItems(range);
    // fill and select ranego in combo box
    QStringList gain = {"10 mm", "6.0 mm", "3.0 mm", "1.5 mm"};
    ui->comboBoxDevices_2->addItems(gain);
    // set default gain
    if (set_default_gain == 0) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN4)");
            rd = 0, max_analog = 32767.0, gain_c = (((float)max_analog-(float)cal_anl)*500.0)/10651.0;
            ui->comboBoxDevices_2->setCurrentIndex(3);
            set_default_gain = 1;
        }
    }
    ui->comboBoxDevices_2->setCurrentIndex(3); // make shure that is set to 1.5 mm (it force showing)
    ui->spinBox_4->setDisabled(true);
    ui->label_23->setStyleSheet("QLabel { color : red; }");
    ui->label_23->setVisible(false);
    //ena = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::autoConnect()
{
    // Auto Connect
    bool arduino_available = false;
    QString arduino_portName;
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
        if (serialPortInfo.hasVendorIdentifier() && serialPortInfo.hasProductIdentifier()) {
            if ((serialPortInfo.productIdentifier() == 4) && (serialPortInfo.vendorIdentifier() == 7855)) {
                arduino_available = true;
                arduino_portName = serialPortInfo.portName();
            }
        }
    }
    if(arduino_available == true) {
        this->device->setPortName(arduino_portName);
        // OTWÓRZ I SKONFIGURUJ PORT:
        if(!device->isOpen())
        {
            if(device->open(QSerialPort::ReadWrite))
            {
                this->device->setBaudRate(QSerialPort::UnknownBaud);
                this->device->setDataBits(QSerialPort::Data8);
                this->device->setParity(QSerialPort::NoParity);
                this->device->setStopBits(QSerialPort::OneStop);
                this->device->setFlowControl(QSerialPort::NoFlowControl);
                // CONNECT:
                connect(this->device, SIGNAL(readyRead()), this, SLOT(readFromPort()));
                this->addToLogs("Połączono z Amslerem.", "green");
                ui->label_4->setStyleSheet("QLabel { color : green; }");
                ui->label_4->setWordWrap(true);
                ui->label_4->setText("Połączono Automatycznie");
                set_default_gain = 0;
            }
            else
            {
                this->addToLogs("Otwarcie portu nie powiodło!", "red");
                ui->label_4->setStyleSheet("QLabel { color : red; }");
                ui->label_4->setText("NIE połaczono");
            }
        }
        else
        {
            this->addToLogs("Port już jest otwarty!", "orange");
            return;
        }
    }
    else {
        this->addToLogs("Amsler nie jest podłączony.", "red");
        ui->label_4->setStyleSheet("QLabel { color : red; }");
        ui->label_4->setText("NIE połaczono");
    }
    if (set_default_gain == 0) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN4)");
            rd = 0, max_analog = 32767.0, gain_c = (((float)max_analog-(float)cal_anl)*500.0)/10651.0;
            ui->comboBoxDevices_2->setCurrentIndex(3);
            set_default_gain = 1;
        }
    }
}

void MainWindow::makePlot()
{
    // give the axes some labels:
    ui->centralWidget_2->xAxis->setLabel("Czas pomiaru");
    // configure plot to have two right axes:
    ui->centralWidget_2->yAxis->setTickLabels(false);
    // left axis mirrors inner right axis
    connect(ui->centralWidget_2->yAxis2, SIGNAL(rangeChanged(QCPRange)), ui->centralWidget_2->yAxis, SLOT(setRange(QCPRange)));
    ui->centralWidget_2->yAxis2->setVisible(true);
    // add two Axis
    ui->centralWidget_2->axisRect()->addAxis(QCPAxis::atRight);
    ui->centralWidget_2->axisRect()->addAxis(QCPAxis::atRight);
    // add paddings to Axis
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 0)->setPadding(25); // add some padding to have space for tags
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 1)->setPadding(25); // add some padding to have space for tags
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2)->setPadding(25); // add some padding to have space for tags
    // add labels
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 0)->setLabel("Zużycie lniowe [µm]");
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 1)->setLabel("Współczynnik tarcia - µ");
    ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2)->setLabel("Siła tarcia [N]");
    // create graphs:
    mGraph1 = ui->centralWidget_2->addGraph(ui->centralWidget_2->xAxis, ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 0));
    mGraph2 = ui->centralWidget_2->addGraph(ui->centralWidget_2->xAxis, ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 1));
    mGraph3 = ui->centralWidget_2->addGraph(ui->centralWidget_2->xAxis, ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2));
    mGraph1->setPen(QPen(QColor(250, 120, 0)));
    mGraph2->setPen(QPen(QColor(0, 180, 60)));
    mGraph3->setPen(QPen(QColor(0, 0, 255)));
    // create time tick on x axes
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    ui->centralWidget_2->xAxis->setTicker(timeTicker);
    // create tags with newly introduced AxisTag class (see axistag.h/.cpp):
    mTag1 = new AxisTag(mGraph1->valueAxis());
    mTag1->setPen(mGraph1->pen());
    mTag2 = new AxisTag(mGraph2->valueAxis());
    mTag2->setPen(mGraph2->pen());
    mTag3 = new AxisTag(mGraph3->valueAxis());
    mTag3->setPen(mGraph3->pen());
    // add legend
    ui->centralWidget_2->legend->setVisible(true);
    ui->centralWidget_2->graph(0)->setName("Zużycie liniowe [µm]");
    ui->centralWidget_2->graph(1)->setName("Współczynnik tarcia");
    ui->centralWidget_2->graph(2)->setName("Siła tarcia [N]");
    // add legend under graph
    QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    ui->centralWidget_2->plotLayout()->addElement(1, 0, subLayout);
    subLayout->setMargins(QMargins(5, 0, 5, 5));
    subLayout->addElement(0, 0, ui->centralWidget_2->legend);
    // change the fill order of the legend, so it's filled left to right in columns:
    ui->centralWidget_2->legend->setFillOrder(QCPLegend::foColumnsFirst);
    // set legend's row stretch factor very small so it ends up with minimum height:
    ui->centralWidget_2->plotLayout()->setRowStretchFactor(1, 0.001);
    //after declare - connect timerSlot() to the timer
    connect(&mDataTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

void MainWindow::on_pushButtonSearch_clicked()
{
    ui->comboBoxDevices->clear();
    this->addToLogs("Szukam urządzeń...", "black");
    QList<QSerialPortInfo> devices;
    devices = QSerialPortInfo::availablePorts();
    for(int i = 0; i < devices.count(); i++)
    {
        this->addToLogs("Znaleziono urządzenie: " + devices.at(i).portName() + " " + devices.at(i).description(), "black");
//this->addToLogs("ID: " + QString::number(devices.at(i).vendorIdentifier()) + " " +  QString::number(devices.at(i).productIdentifier()));
        ui->comboBoxDevices->addItem(devices.at(i).portName() + "\t" + devices.at(i).description());
    }
}

void MainWindow::addToLogs(QString message, QString color)
{
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss");
    ui->textEditLogs->setTextColor(QColor(color));
    ui->textEditLogs->append(currentDateTime + "  " + message);
}

void MainWindow::sendMessageToDevice(QString message)
{
    if(this->device->isOpen() && this->device->isWritable())
    {
        this->addToLogs("Wysyłam do Amslera: " + message, "blue");
        this->device->write(message.toStdString().c_str());
        this->device->flush();
    }
    else
    {
        this->addToLogs("Port nie jest otwarty!", "red");
        autoConnect(); // just in case
    }
}

void MainWindow::on_pushButtonConnect_clicked()
{
    if(ui->comboBoxDevices->count() == 0)
    {
        this->addToLogs("Nie wykryto żadnych urządzeń!", "black");
        return;
    }
    QString comboBoxQString = ui->comboBoxDevices->currentText();
    QStringList portList = comboBoxQString.split("\t");
    QString portName = portList.first();
    this->device->setPortName(portName);
    // open and configure
    if(!device->isOpen())
    {
        if(device->open(QSerialPort::ReadWrite))
        {
            this->device->setBaudRate(QSerialPort::UnknownBaud);
            this->device->setDataBits(QSerialPort::Data8);
            this->device->setParity(QSerialPort::NoParity);
            this->device->setStopBits(QSerialPort::OneStop);
            this->device->setFlowControl(QSerialPort::NoFlowControl);
            // CONNECT:
            connect(this->device, SIGNAL(readyRead()), this, SLOT(readFromPort()));
            this->addToLogs("Otwarto port szeregowy.", "green");
            ui->label_4->setStyleSheet("QLabel { color : green; }");
            ui->label_4->setText("Połaczono");
            // set default gain manually by clicking connect
            if (set_default_gain == 0) {
                if(this->device->isOpen()) {
                    this->sendMessageToDevice("(GAIN4)");
                    rd = 0, max_analog = 32767.0, gain_c = (((float)max_analog-(float)cal_anl)*500.0)/10651.0;
                    ui->comboBoxDevices_2->setCurrentIndex(3);
                    set_default_gain = 1;
                }
            }
        }
        else
        {
            this->addToLogs("Otwarcie portu nie powiodło!", "red");
        }
    }
    else
    {
        this->addToLogs("Port już jest otwarty!", "orange");
        return;
    }
}

void MainWindow::readFromPort()
{
    while(this->device->canReadLine())
    {
        QString line = this->device->readLine();
        //qDebug() << line;
        if (rd == 1) {
            rec_cnt++;
            QStringList fields = line.split(" ");
            QString Analog = fields[0];
            QString Digital = fields[1];
            Analog_ = Analog.toInt();
            Digital_ = Digital.toInt();
            // zero calibration
            Digital_c = Digital_;
            //Analog_c = Analog_ - cal_anl; // analog read - calibration
            Analog_c = -(map(Analog_, cal_anl, max_analog, 0.0, gain_c)); // inverted results
            //qDebug() << Analog_c;
            //qDebug() << Digital_c;
            rd = 0;
        }
        QString terminator = "\r";
        int pos = line.lastIndexOf(terminator);
        //qDebug() << line.left(pos);
        this->addToLogs(line.left(pos), "darkviolet");
    }
}

void MainWindow::on_pushButtonCloseConnection_clicked()
{
    if(this->device->isOpen())
    {
        this->device->close();
        ui->label_4->setStyleSheet("QLabel { color : red; }");
        ui->label_4->setText("NIE połaczono");
        this->addToLogs("Zamknięto połączenie.", "red");
        ena = 0;
        set_default_gain = 0;
    }
    else
    {
        this->addToLogs("Port nie jest otwarty!", "red");
        return;
    }
}

void MainWindow::on_pushButtonLedOn_clicked() // start
{
    if(this->device->isOpen()) {
        mDataTimer.start(interval); // just in case (on calibration first use)
        timer2->restart();
        ena = 1;
        ui->pushButton_4->setDisabled(true);
        ui->pushButtonLedOn->setDisabled(true);
        ui->pushButtonLedOff->setDisabled(false);
        ui->pushButtonLedOff_2->setDisabled(true);
        ui->checkBox_8->setDisabled(true);
    }
    else this->addToLogs("Port nie jest otwarty!", "red");
}

void MainWindow::on_pushButtonLedOff_clicked() // pause
{
    if(this->device->isOpen()) {
        mDataTimer.stop();
        last_time += timer2->elapsed()/1000.0; // last time stamp
        timer2->restart();
        ena = 0;
        ui->pushButton_4->setDisabled(true);
        ui->pushButtonLedOn->setDisabled(false);
        ui->pushButtonLedOff->setDisabled(true);
        ui->pushButtonLedOff_2->setDisabled(false);
        ui->checkBox_8->setDisabled(false);
        if (ui->checkBox_8->isChecked()) ui->checkBox_8->setChecked(0);
    }
    else this->addToLogs("Port nie jest otwarty!", "red");
}

void MainWindow::on_pushButtonLedOff_2_clicked() // stop
{
    if(this->device->isOpen()) {
        first_save_file = 0;
        mDataTimer.stop();
        last_time += timer2->elapsed()/1000.0; // last time stamp
        last_reset = last_time;
        ui->centralWidget_2->graph(0)->data()->removeBefore(last_reset);
        ui->centralWidget_2->graph(1)->data()->removeBefore(last_reset);
        ui->centralWidget_2->graph(2)->data()->removeBefore(last_reset);
        ui->centralWidget_2->xAxis->moveRange(-last_reset);
        ui->centralWidget_2->replot();
        timer2->restart();
        ena = 0;
        ui->pushButton_4->setDisabled(false);
        ui->pushButtonLedOff->setDisabled(true);
        ui->pushButtonLedOff_2->setDisabled(true);
        ui->checkBox_8->setDisabled(false);
        if (!ui->checkBox_8->isChecked()) ui->pushButtonLedOn->setDisabled(false);
        else ui->pushButtonLedOn->setDisabled(true);
    }
    else this->addToLogs("Port nie jest otwarty!", "red");
}

void MainWindow::on_checkBox_8_toggled(bool checked)
{
    if (checked == 1 && ena == 0) {
        ui->pushButton_4->setDisabled(false);
        ui->pushButtonLedOn->setDisabled(true);
        file_name = "";
    }
    if (checked == 0 && ena == 0) {
        ui->pushButtonLedOn->setDisabled(false);
        ui->pushButtonLedOn->setDisabled(false);
    }
}

void MainWindow::on_pushButton_clicked()
{
    ui->textEditLogs->clear();
}

void MainWindow::on_pushButton_2_clicked() // cal digital button
{
    int ena_old = 0;
    if(ena == 1) {
        ena = 0;
        ena_old = 1;
    }
    if(this->device->isOpen()) this->sendMessageToDevice("(EE)"), rd = 0;
    else this->addToLogs("Port nie jest otwarty!", "red");
    if(ena_old == 1) ena = 1;
    else ena_old = 0; // just in case (is declaring as local)
    if(this->device->isOpen()) ui->label_d->setStyleSheet("QLabel { color : green; }");
    QTimer::singleShot(1000, [this]() { ui->label_d->setStyleSheet("QLabel { color : black; }"); } );
    if (checked_test == 0) {
        on_pushButton_6_toggled(1);
        on_pushButton_6_toggled(0);
    }
}

void MainWindow::on_pushButton_3_clicked() // cal analog button
{
    int ena_old = 0;
    if(ena == 1) {
        ena = 0;
        ena_old = 1;
    }
    cal_anl = 0;
    cal_anl = Analog_;
    if(ena_old == 1) ena = 1;
    else ena_old = 0; // just in case (is declaring as local)
    if(this->device->isOpen()) ui->label_a->setStyleSheet("QLabel { color : green; }");
    QTimer::singleShot(1000, [this]() { ui->label_a->setStyleSheet("QLabel { color : black; }"); } );
    if (checked_test == 0) {
        on_pushButton_6_toggled(1);
        on_pushButton_6_toggled(0);
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if (file_name <= 0) first_save_file = 1;
    else first_save_file = 0;
    file_name = QFileDialog::getSaveFileName(this, "Zapisz plik jako", QDir::homePath(), tr("Pliki tekstowe (*.txt *.csv)"));
    if (file_name > 0) {
        QMessageBox::information(this, "Ustawiona ścieżka", file_name);
        ui->pushButtonLedOn->setDisabled(false);
        ui->checkBox_8->setDisabled(true);
    }
}

void MainWindow::on_checkBox_4_toggled(bool checked)
{
    if (checked == 1) {
        scroll_time = 1;
    }
    else {
        scroll_time = 0;
    }
}

void MainWindow::on_checkBox_toggled(bool checked) // Analog chart
{
    if (checked == 0) {
        ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 0)->setVisible(false);
        mGraph1->setVisible(false);
        disable_tag_1 = 1;
        mTag1->updatePosition(10000);
    }
    else {
        ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 0)->setVisible(true);
        mGraph1->setVisible(true);
        disable_tag_1 = 0;
    }
    timerSlot(); // replot legend
    ui->centralWidget_2->replot();
}

void MainWindow::on_checkBox_2_toggled(bool checked) // Digital chart
{
    if (checked == 0) {
        ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 1)->setVisible(false);
        mGraph2->setVisible(false);
        disable_tag_2 = 1;
        mTag2->updatePosition(10000);
    }
    else {
        ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 1)->setVisible(true);
        mGraph2->setVisible(true);
        disable_tag_2 = 0;
    }
    timerSlot(); // replot legend
    ui->centralWidget_2->replot();
}

void MainWindow::on_checkBox_3_toggled(bool checked) // force chart
{
    if (checked == 0) {
        ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2)->setVisible(false); // hide axis
        mGraph3->setVisible(false); // hide graph
        disable_tag_3 = 1;
        mTag3->updatePosition(10000); // move out tag
    }
    else {
        ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2)->setVisible(true);
        mGraph3->setVisible(true);
        disable_tag_3 = 0;
    }
    timerSlot(); // replot legend
    ui->centralWidget_2->replot();
}

void MainWindow::on_checkBox_5_toggled(bool checked)
{
    auto_disable = checked;
    if (checked == 0) {
        QList<QCPAxis *> axis;
        axis.append(ui->centralWidget_2->axisRect()->axis(QCPAxis::atBottom));
        axis.append(ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 0));
        axis.append(ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 1));
        axis.append(ui->centralWidget_2->axisRect()->axis(QCPAxis::atRight, 2));
        ui->centralWidget_2->axisRect()->setRangeDragAxes(axis);
        ui->centralWidget_2->axisRect()->setRangeZoomAxes(axis);
        ui->centralWidget_2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iSelectLegend);
    }
    else {
        ui->centralWidget_2->setInteractions(QCP::iSelectOther);
        mGraph1->rescaleValueAxis(false, true);
        mGraph2->rescaleValueAxis(false, true);
        mGraph3->rescaleValueAxis(false, true);
        ui->centralWidget_2->replot();
    }
}

void MainWindow::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
  // Rename a graph by double clicking on its legend item
  Q_UNUSED(legend)
  if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
  {
    QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
    bool ok;
    QString newName = QInputDialog::getText(this, "Zmień nazwę", "Nowa nazwa:", QLineEdit::Normal, plItem->plottable()->name(), &ok);
    if (ok)
    {
      plItem->plottable()->setName(newName);
      ui->centralWidget_2->replot();
    }
  }
}

void MainWindow::timerSlot()
{
    if (set_default_gain == 0) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN4)");
            rd = 0, max_analog = 32767.0, gain_c = (((float)max_analog-(float)cal_anl)*500.0)/10651.0;
            ui->comboBoxDevices_2->setCurrentIndex(3);
            set_default_gain = 1;
        }
    }
    ui->label_3->setText(QString::number(mDataTimer.remainingTime()) + " ms");
    ui->centralWidget_2->legend->clearItems();
    const int iPlottableCount = ui->centralWidget_2->plottableCount();
    for (int iPlottable = 0; iPlottable < iPlottableCount; ++iPlottable)
    {
        if (QCPAbstractPlottable* pPlottable = ui->centralWidget_2->plottable(iPlottable))
        {
            const bool bVisible = pPlottable->realVisibility();
            if (bVisible)
                pPlottable->addToLegend();
        }
    }
    if (ena == 1) {
        // send data request
        this->sendMessageToDevice("(RD)"), rd = 1;
        // display data in debug menu
        ui->label_d->setText(QString::number((float)Digital_c/100.0) + "mm");
        char buf[10];
        sprintf (buf, "%.2fum", Analog_c);
        ui->label_a->setText(buf);
        // calculate two new data points (for chart):
        key = last_time + (timer2->elapsed()/1000.0); // time elapsed since start - in seconds
        send_cnt++;
        //calc Friction Force [N]
        if (Digital_c >= 0) force = ((double)Digital_c/100.0), ui->label_23->setVisible(false);
        else {
            force = 0;
            ui->label_23->setVisible(true);
        }
        // calc range
        if (ui->comboBoxDevices_3->currentIndex() == 0) force = (10 * (double)force)/(80.0);
        if (ui->comboBoxDevices_3->currentIndex() == 1) force = (50 * (double)force)/(80.0);
        if (ui->comboBoxDevices_3->currentIndex() == 2) force = (100 * (double)force)/(80.0);
        if (ui->comboBoxDevices_3->currentIndex() == 3) force = (150 * (double)force)/(80.0);
        force = (200.0 * (double)force)/((double)r * 2000.0);
        if (mass <= 0) mass = 1; // for no dividing by 0
        u = (double)force / (double)mass;
        // print foce and friction coefficient on the label
        ui->label_19->setText(QString::number(force) + " N");
        ui->label_20->setText(QString::number(u));
        // calculate and add a new data point to each graph:
        mGraph1->addData(key, Analog_c); // Penetration Depth [µm]
        mGraph2->addData(key, u); // µ
        mGraph3->addData(key, force); // Friction Force [N]
        if (auto_disable == 1) {
            // make key axis range scroll with the data:
            mGraph1->rescaleValueAxis(false, true);
            mGraph2->rescaleValueAxis(false, true);
            mGraph3->rescaleValueAxis(false, true);
        }
        // update the vertical axis tag positions and texts to match the rightmost data point of the graphs:
        double graph1Value = mGraph1->dataMainValue(mGraph1->dataCount()-1);
        double graph2Value = mGraph2->dataMainValue(mGraph2->dataCount()-1);
        double graph3Value = mGraph3->dataMainValue(mGraph3->dataCount()-1);
        if (disable_tag_1 == 0) mTag1->updatePosition(graph1Value), mTag1->setText(QString::number(graph1Value, 'f', 2)+" um");
        if (disable_tag_2 == 0) mTag2->updatePosition(graph2Value), mTag2->setText(QString::number(graph2Value, 'f', 2)+"");
        if (disable_tag_3 == 0) mTag3->updatePosition(graph3Value), mTag3->setText(QString::number(graph3Value, 'f', 2)+" N");
        // make key axis range scroll with the data (at a constant range size):
        if (scroll_time == 0) ui->centralWidget_2->xAxis->setRange(key, key-last_reset-1, Qt::AlignRight); // no scrolling data
        else ui->centralWidget_2->xAxis->setRange(key, time_int, Qt::AlignRight);
        ui->centralWidget_2->replot();
        // save log in to the file
        if (file_name != "") {
            QFile file(file_name);
            if(!file.open(QFile::WriteOnly | QFile::Append)) return;
            else {
                QTextStream stream(&file);
                if(first_save_file == 1) {
                    first_save_file = 0;
                    stream << "Czas_[s] " << "Zużycie_liniowe_[µm] " << "Współczynnik_tarcia_[µ]" << "\n";
                }
                stream << key-last_reset << " " << Analog_c << " " << u << "\n";
                stream.flush();
                file.close();
            }
        }
    }
    // display time elapsed
    int secs = (int)key-last_reset;
    int h = secs / 3600;
    int m = ( secs % 3600 ) / 60;
    int s = ( secs % 3600 ) % 60;
    QTime t(h, m, s);
    ui->timeEdit_2->setTime(t);
    //display time remaining
    int totalRemainingTime = ((ui->timeEdit->time().hour() * 60.0 * 60.0) + (ui->timeEdit->time().minute() * 60.0) + ui->timeEdit->time().second()) - secs;
    int h2 = totalRemainingTime / 3600;
    int m2 = ( totalRemainingTime % 3600 ) / 60;
    int s2 = ( totalRemainingTime % 3600 ) % 60;
    QTime t2(h2, m2, s2);
    ui->timeEdit_3->setTime(t2);
    // display distance
    ui->label_15->setText(QString::number(round((float)(v * secs))) + " m");
    // calculate lost data packets
    double percent = ((double)rec_cnt/(double)send_cnt)*100.0;
    ui->label_5->setText(QString::number(round(percent)) + "%");
    //reconnect
    uint8_t int_check = 2; // 2 sec. checking a data stream (choose from: 2, 3, 4, 5, 6, 10 sec)
    if ((double)send_cnt >= ((1.0/(double)interval)*1000.0)*int_check) rec_cnt = 0, send_cnt = 0;
    if (t.second() % int_check) {
        if (ena_timer == 1) {
            ena_timer = 0;
            if (percent < 90.0 && checked_test == 0) {
                this->device->close();
                autoConnect();
            }
        }
    }
    else ena_timer = 1;
    //compare time seted and elapsed
    if (t.hour() == ui->timeEdit->time().hour() && t.minute() == ui->timeEdit->time().minute() && t.second() >= ui->timeEdit->time().second()) {
        if (ena == 1) on_pushButtonLedOff_clicked();
        // raport time :)
    }
}

void MainWindow::mousePress()
{
  // if an axis is selected, only allow the direction of that axis to be dragged
  // if no axis is selected, both directions may be dragged
  if (ui->centralWidget_2->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->centralWidget_2->axisRect()->setRangeDrag(ui->centralWidget_2->xAxis->orientation());
  else if (ui->centralWidget_2->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->centralWidget_2->axisRect()->setRangeDrag(ui->centralWidget_2->yAxis->orientation());
  else
    ui->centralWidget_2->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::mouseWheel()
{
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed
  if (ui->centralWidget_2->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->centralWidget_2->axisRect()->setRangeZoom(ui->centralWidget_2->xAxis->orientation());
  else if (ui->centralWidget_2->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->centralWidget_2->axisRect()->setRangeZoom(ui->centralWidget_2->yAxis->orientation());
  else
    ui->centralWidget_2->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}

void MainWindow::on_spinBox_2_valueChanged(int arg1)
{
    interval = (1.0/arg1)*1000;
    mDataTimer.start(interval);
}

void MainWindow::on_pushButton_5_clicked() // generate PDF
{
    ui->textBrowser->clear();
    mTag1->updatePosition(10000); // move out tag
    mTag2->updatePosition(10000); // move out tag
    mTag3->updatePosition(10000); // move out tag
    ui->centralWidget_2->replot();
    QTextCursor cursor = ui->textBrowser->textCursor();
    double width = 600;
    double height = 300;
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), QCPDocumentObject::generatePlotFormat(ui->centralWidget_2, width, height));
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss");
    cursor.insertText(QString('\n') + QString('\n') + "Data utworzenia raportu: " + currentDateTime + cursor.EndOfLine);
    cursor.insertText("Nazwa projektu: " + ui->lineEdit->text() + cursor.EndOfLine);
    cursor.insertText("Rodzaj próbki: " + ui->lineEdit_2->text() + cursor.EndOfLine);
    cursor.insertText("Rodzaj powłoki: " + ui->lineEdit_3->text() + cursor.EndOfLine);
    cursor.insertText("Smarowanie: " + ui->lineEdit_4->text() + cursor.EndOfLine);
    cursor.insertText("Średnica próbki [mm]: " + ui->spinBox_5->text() + cursor.EndOfLine);
    cursor.insertText("Czas wykonywania: " + ui->timeEdit_2->text() + cursor.EndOfLine);
    cursor.insertText("Droga tarcia [m]: " + ui->label_15->text() + cursor.EndOfLine);
    cursor.insertText("Obciążenie [N]: " + ui->spinBox_3->text() + cursor.EndOfLine);
    cursor.insertText("Próbkowanie [Hz]: " + ui->spinBox_2->text() + cursor.EndOfLine);
    ui->textBrowser->setTextCursor(cursor);
    ui->textBrowser->setMinimumSize(640, 500);
}

void MainWindow::on_pushButton_7_clicked() // zapisz PDF
{
    QString fileName = QFileDialog::getSaveFileName(this, "Zapisz plik jako", qApp->applicationDirPath(), "*.pdf");
    if (!fileName.isEmpty())
    {
      QPrinter printer;
      printer.setOutputFormat(QPrinter::PdfFormat);
      printer.setOutputFileName(fileName);
      QMargins pageMargins(20, 20, 20, 20);
  #if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
      printer.setFullPage(false);
      printer.setPaperSize(QPrinter::A4);
      printer.setOrientation(QPrinter::Portrait);
      printer.setPageMargins(pageMargins.left(), pageMargins.top(), pageMargins.right(), pageMargins.bottom(), QPrinter::Millimeter);
  #else
      QPageLayout pageLayout;
      pageLayout.setMode(QPageLayout::StandardMode);
      pageLayout.setOrientation(QPageLayout::Portrait);
      pageLayout.setPageSize(QPageSize(QPageSize::A4));
      pageLayout.setUnits(QPageLayout::Millimeter);
      pageLayout.setMargins(QMarginsF(pageMargins));
      printer.setPageLayout(pageLayout);
  #endif
      ui->textBrowser->document()->setPageSize(printer.pageRect().size());
      ui->textBrowser->document()->print(&printer);
    }
}

void MainWindow::on_comboBoxDevices_2_activated(int index)
{
    // gain_c = ((32767-cal_anl)*(float)measured_distance_gain_1)/measured_ADC_val_gain_1
    //measured_distance_gain_1 - zmierzona odległość
    //measured_ADC_val_gain_1 - otrzymana warotść
    if (index == 0) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN1)");
            rd = 0, max_analog = 26214.0, gain_c = 10000.0; // not calibrated, is rounded up.
        }
        else this->addToLogs("Port nie jest otwarty!", "red");
    }
    if (index == 1) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN2)");
            rd = 0, max_analog = 32767.0, gain_c = (((32767.0-(float)cal_anl)*500.0)/10651.0)*4.0; // 6138
        }
        else this->addToLogs("Port nie jest otwarty!", "red");
    }
    if (index == 2) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN3)");
            rd = 0, max_analog = 32767.0, gain_c = (((32767.0-(float)cal_anl)*500.0)/10651.0)*2.0; // 3069
        }
        else this->addToLogs("Port nie jest otwarty!", "red");
    }
    if (index == 3) {
        if(this->device->isOpen()) {
            this->sendMessageToDevice("(GAIN4)");
            rd = 0, max_analog = 32767.0, gain_c = ((32767.0-(float)cal_anl)*500.0)/10651.0; // 1534
        }
        else this->addToLogs("Port nie jest otwarty!", "red");
    }
}

void MainWindow::on_spinBox_4_valueChanged(int arg1)
{
    if (ui->radioButton_3->isChecked() == 1 && rec == 0) {
        v = r * rpm * 0.10472;
        distance = (double)arg1/(double)v;
        int secs = (int)round(distance);
        int h = secs / 3600;
        int m = ( secs % 3600 ) / 60;
        int s = ( secs % 3600 ) % 60;
        QTime t(h, m, s);
        ui->timeEdit->setTime(t);
    }
}

void MainWindow::on_timeEdit_userTimeChanged(const QTime &time)
{
    if (ui->radioButton_2->isChecked() == 1 && rec == 0) {
        v = r * rpm * 0.10472;
        distance = v * ((time.hour() * 60.0 * 60.0) + (time.minute() * 60.0) + time.second());
        ui->spinBox_4->setValue((uint32_t)round(distance));
    }
}

void MainWindow::on_checkBox_6_toggled(bool checked)
{
    if (checked == 1) rpm = 400.0; // fast (386 measured)
    else rpm = (double)rpm/2.0; // slow
    ReCalc();
}

void MainWindow::ReCalc()
{
    rec = 1;
    v = r * rpm * 0.10472;
    double old_distance = (uint32_t)(ui->spinBox_4->value());
    distance = (double)old_distance/(double)v;
    int secs = (int)round(distance);
    int h = secs / 3600;
    int m = ( secs % 3600 ) / 60;
    int s = ( secs % 3600 ) % 60;
    QTime t(h, m, s);
    ui->timeEdit->setTime(t);
    distance = (double)v * (double)secs;
    ui->spinBox_4->setValue((uint32_t)round(distance));
    rec = 0;
}

void MainWindow::on_radioButton_2_toggled(bool checked)
{
    if (checked == 1) {
        ui->timeEdit->setDisabled(false);
        ui->spinBox_4->setDisabled(true);
    }
    else {
        ui->timeEdit->setDisabled(true);
        ui->spinBox_4->setDisabled(false);
    }
}

void MainWindow::on_spinBox_3_valueChanged(int arg1) // force and range
{
    mass = arg1; // mass [N]
}

void MainWindow::on_spinBox_5_valueChanged(int arg1)
{
    r = ((double)arg1 * 0.001)/2.0;
    ReCalc();
}

void MainWindow::on_timeEdit_4_userTimeChanged(const QTime &time)
{
    time_int = ((time.hour() * 60.0 * 60.0) + (time.minute() * 60.0) + time.second());
}

void MainWindow::on_checkBox_7_toggled(bool checked)
{
    if (checked == 1) ui->centralWidget_2->legend->setVisible(true);
    else ui->centralWidget_2->legend->setVisible(false);
    ui->centralWidget_2->replot();
}

void MainWindow::on_pushButton_6_toggled(bool checked) // test button
{
    if (checked) {
        checked_test = 1;
        old_ena = ena;
        ena = 1;
        timerSlot();
        if (!mDataTimer.isActive()) mDataTimer.start(interval);
    }
    else ena = old_ena, checked_test = 0;
}
