/*
 *
 * 2019 © Copyright Mateusz Patyk
 *
 * Autor: Mateusz Patyk
 * Kontakt: matpatyk@gmail.com
 *
 * Seria:
 * Qt - Tworzenie interfejsów aplikacji na urządzenia mobilne i komputery personalne wykorzystujące
 * komunikację poprzez port szeregowy i Bluetooth.
 *
 * Dla: Forbot, https://forbot.pl/blog/
 * Część: druga
 *
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTextDocument>
#include <QFileDialog>
#include "qcustomplot.h"
#include "axistag.h"
#include "qcpdocumentobject.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonSearch_clicked();
    void on_pushButtonConnect_clicked();
    void readFromPort();
    void on_pushButtonCloseConnection_clicked();
    void on_pushButtonLedOn_clicked();
    void on_pushButtonLedOff_clicked();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void makePlot();
    void autoConnect();
    void on_pushButton_4_clicked();
    void on_checkBox_4_toggled(bool checked);
    void on_checkBox_toggled(bool checked);
    void on_checkBox_2_toggled(bool checked);
    void on_checkBox_3_toggled(bool checked);
    void on_checkBox_5_toggled(bool checked);
    void on_timeEdit_userTimeChanged(const QTime &time);
    void timerSlot();
    void mousePress();
    void mouseWheel();
    void on_spinBox_2_valueChanged(int arg1);
    void on_pushButton_5_clicked();
    void on_pushButton_7_clicked();
    void on_comboBoxDevices_2_activated(int index);
    void on_spinBox_4_valueChanged(int arg1);
    void on_checkBox_6_toggled(bool checked);
    void on_radioButton_2_toggled(bool checked);
    void on_spinBox_3_valueChanged(int arg1);
    void on_spinBox_5_valueChanged(int arg1);
    void ReCalc();
    void on_timeEdit_4_userTimeChanged(const QTime &time);
    void on_pushButtonLedOff_2_clicked();
    void legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item);
    void on_checkBox_7_toggled(bool checked);
    void on_checkBox_8_toggled(bool checked);
    void on_pushButton_6_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    void addToLogs(QString message, QString color = "black");
    QSerialPort *device;
    void sendMessageToDevice(QString message);
    QPointer<QCPGraph> mGraph1;
    QPointer<QCPGraph> mGraph2;
    QPointer<QCPGraph> mGraph3;
    AxisTag *mTag1;
    AxisTag *mTag2;
    AxisTag *mTag3;
    QTimer mDataTimer;
};

#endif // MAINWINDOW_H
