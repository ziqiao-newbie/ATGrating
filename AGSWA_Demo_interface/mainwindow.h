#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "agswacontrol.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonConnect_clicked();

    void on_pushButtonGetbasicInfo_clicked();

    void on_pushButtonStartContinousWL_clicked();

    void on_pushButtonStop_clicked();

    void on_pushButtonGetParameter_clicked();

    void on_pushButtonObtainDetailsAndConfig_clicked();

    void on_pushButtonStartContSpectral_clicked();

private:
    Ui::MainWindow *ui;
    AgswaControl * agswaControl;
};
#endif // MAINWINDOW_H
