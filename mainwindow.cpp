#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "hydratooldialog.h"
#include "version.h"

#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QString title_ver_info;

    // Load the embedded font.
    QString fontPath = ":/fonts/LiberationSans-Regular.ttf";
    QFontDatabase::addApplicationFont(fontPath);
    QFont font("Liberation Sans",8);
    this->setFont(font);
    QFontDatabase db1;

    title_ver_info = "hydratool v";
    title_ver_info.append(VER_FILEVERSION_STR);
    title_ver_info.append(VER_DATE_INFO_STR);
    title_ver_info.append(" (Based on Qt");
    title_ver_info.append(QT_VERSION_STR);
    title_ver_info.append(")");

    ui->setupUi(this);
    this->setWindowTitle(title_ver_info);

    hydratool_log = new hydratooldialog(0,
                                        ui->dockWidget_hydratool_directdisk,
                                        ui->dockWidget_terminal_directdisk,
                                        ui->dockWidget_terminal,
                                        ui->dockWidget_find);
    setCentralWidget(hydratool_log);
    readSettings();
}

MainWindow::~MainWindow()
{
    writeSettings();
    delete hydratool_log;
    delete ui;
}

/*****************************************************************************/
/* Protected methods */
/*****************************************************************************/

void MainWindow::readSettings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();
}

