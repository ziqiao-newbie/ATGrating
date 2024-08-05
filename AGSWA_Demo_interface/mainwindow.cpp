#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    agswaControl = new AgswaControl();
    ui->lineEditIp->setText("192.168.1.12");
}

MainWindow::~MainWindow()
{
    delete ui;
    agswaControl->closeConnect();
    delete agswaControl;
}


void MainWindow::on_pushButtonConnect_clicked()
{
    agswaControl->connectIMON(ui->lineEditIp->text());
}


void MainWindow::on_pushButtonGetbasicInfo_clicked()
{
    agswaControl->getDeviceBasicInfo();
}


void MainWindow::on_pushButtonStartContinousWL_clicked()
{
    agswaControl->startContiunousCalculateWl(ui->spinBoxContWLFreq->value());
}


void MainWindow::on_pushButtonStop_clicked()
{
    agswaControl->stopContinuous();
}


void MainWindow::on_pushButtonGetParameter_clicked()
{
    agswaControl->getConfigFromImonBoard();
}


void MainWindow::on_pushButtonObtainDetailsAndConfig_clicked()
{
    agswaControl->getDeviceDetailAndConfiguration();
}


void MainWindow::on_pushButtonStartContSpectral_clicked()
{
    agswaControl->startContunousSpectralData(ui->spinBoxSpectrumFreq->value());
}

