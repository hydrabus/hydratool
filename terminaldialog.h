#ifndef TERMINALDIALOG_H
#define TERMINALDIALOG_H

#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QSemaphore>
#include <QTimer>
#include <QDockWidget>

#include "settingsdialog.h"
#include "directdiskdialog.h"

namespace Ui {
class TerminalDialog;
}

class TerminalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TerminalDialog(QWidget *parent = 0);
    ~TerminalDialog();
    void txData(const QString &text_data);
    void displayData(const QString &data_string);
    void clear(void);
    Qt::CheckState PCdateTimeCheckState(void);
    void setPCdateTimeCheckState(Qt::CheckState checked);

private slots:
    void hideEvent(QHideEvent* event);
    void reject();

    void openSerialPort();
    void closeSerialPort();
    void raw_data();
    void rxData();
    void handleError(QSerialPort::SerialPortError error);
    void settings_hide(bool change_settings);

    void on_clearButton_clicked();

    void on_saveButton_clicked();

    void on_lineEdit_lineExecuted(QString text);

    void on_checkBoxPCdateTime_clicked(bool checked);

    void on_lineEdit_returnPressed();

    void keyPressEvent(QKeyEvent *e);

    void on_ConfigureButton_clicked();

    void on_actionConnect_clicked(bool checked);

    void serialport_timer_update();

    void on_saveDirectToDisk_clicked();

    void on_checkBoxHexAsc_clicked(bool checked);

    void on_sendFileButton_clicked();

private:
    Ui::TerminalDialog *ui;
    SettingsDialog *settingsCom;
    QSerialPort *serial;
    QSerialPortInfo *serialinfo;
    DirectDiskDialog *directdisk;

    /* Extern qdockWidget */
    QDockWidget *dockwidget_directdisk_ext;

    QString savefile_path;
    QString lineEdit_text;
    bool PCdateTimeEnabled;
    QString FullTextLine;

    QSemaphore sem;
    QTimer serialport_timer;
    QSemaphore serialport_timer_sem;
    unsigned long serialport_nb_bytes_tx;
    unsigned long serialport_nb_bytes_rx;
    unsigned long serialport_nb_packets_rx;
    bool AutoScrollEnabled;
    bool itsOkToClose;
    bool HexAscii;
    QString loadfile_path;

private:
    void readSettings();
    void writeSettings();
    bool readBase(const char * str, int * value, int base, int size);
    int convExtendedToString(const char* str_in, char* str_out, int str_in_len);

signals:
    void getData(const QString text_data);

};

#endif // TERMINALDIALOG_H
