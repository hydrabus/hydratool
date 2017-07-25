#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QtGlobal>

#include <QMainWindow>

#include "hydratooldialog.h"

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void readSettings();
    void writeSettings();

    Ui::MainWindow *ui;
    QTabWidget *tab;

    hydratooldialog *hydratool_log;
    QPlainTextEdit *console_find;
};

#endif // MAINWINDOW_H
