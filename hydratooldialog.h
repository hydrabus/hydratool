#ifndef HYDRATOOLDIALOG_H
#define HYDRATOOLDIALOG_H

#include <QtCore/QtGlobal>

#include <QDialog>

#include <QtSerialPort/QSerialPort>

#include <QSemaphore>
#include <QTime>
#include <QTimer>
#include <QDockWidget>

#include "console.h"
#include "settingsdialog.h"
#include "terminaldialog.h"
#include "directdiskdialog.h"

#include "sniff/sniff.h"

namespace Ui {
  class hydratooldialog;
}

#define SETTINGS_FILENAME "hydratool.ini"
#define TEMP_STRING_LEN (65536*3)

class hydratooldialog : public QDialog
{
    Q_OBJECT

public:
    explicit hydratooldialog(QWidget *parent = 0,
                             QDockWidget *dockwidget_hydratool_directdisk = 0,
                             QDockWidget *dockwidget_terminal_directdisk = 0,
                             QDockWidget *dockwidget_terminal = 0,
                             QDockWidget *dockwidget_find = 0);
    ~hydratooldialog();

private slots:
    void hideEvent(QHideEvent* event);

    void reject();

    void openSerialPort();
    void closeSerialPort();
    void readData();

    void handleError(QSerialPort::SerialPortError error);

    void on_actionHex_toggled(bool arg1);

    void on_actionClear_triggered();

    void on_actionAutoScroll_toggled(bool arg1);

    void on_actionAscii_toggled(bool arg1);

    void on_actionRaw_toggled(bool arg1);

    void on_actionPCdateTime_toggled(bool arg1);

    void settings_hide(bool change_settings);

    void on_loadFile_clicked();

    void on_saveFile_clicked();

    void terminal_getData(const QString &text_data);

    void on_historyDepthLines_returnPressed();

    void serialport_timer_update();

    void on_actionChanTerminal_triggered();

    void on_actionResync_triggered();

    void on_actionConnectDisconnect();

    void on_saveDirectToDisk_clicked();

    void on_findValue_returnPressed();

    void on_findPredefBox_currentIndexChanged(int index);

    void on_findLiveRefreshCheckBox_clicked(bool checked);

private:
    void readSettings();
    void writeSettings();
    void initActionsConnections();
    void raw_data(void);
    void sniff_trace(QIODevice *stream, int decode_nb_frames);

private:
    Ui::hydratooldialog *ui;
    QPlainTextEdit *console_find; /* console_search */
    /* Extern qdockWidget */
    QDockWidget *dockwidget_hydratool_directdisk_ext;
    QDockWidget *dockwidget_terminal_directdisk_ext;
    QDockWidget *dockwidget_find_ext;
    QDockWidget *dockwidget_terminal_ext;

    Console *console;
    SettingsDialog *settingsCom;
    QSerialPort *serial;
    QSerialPortInfo *serialinfo;
    TerminalDialog *terminal_chan;
    DirectDiskDialog *directdisk;

    t_sniff_frame_state sniff_frame_state;
    t_sniff_frame sniff_frame;
    QByteArray qb_data;

    bool TraceIniEnabled;
    bool SYSEnabled;
    bool RawEnabled;

    bool PCdateTimeEnabled;
    bool AsciiEnabled;
    bool HexEnabled;

    bool AutoScrollEnabled;
    char temp_string[TEMP_STRING_LEN+1];

    QString loadfile_path;
    QString savefile_path;
    QSemaphore sem;
    bool itsOkToClose;
    QTimer serialport_timer;
    QSemaphore serialport_timer_sem;
    unsigned long serialport_nb_bytes_tx;
    unsigned long serialport_nb_bytes_rx;
    unsigned long serialport_nb_packets_rx;

    bool findLiveRefresh;
    QString findText;
};

#endif // HYDRATOOLDIALOG_H
