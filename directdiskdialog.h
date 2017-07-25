#ifndef DIRECTDISKDIALOG_H
#define DIRECTDISKDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QSemaphore>
#include <QFile>

#include "sniff/sniff.h"

namespace Ui {
class DirectDiskDialog;
}

class DirectDiskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirectDiskDialog(QWidget *parent = 0);
    ~DirectDiskDialog();
    void putData(const t_sniff_frame *in_frame);
    void putData(const char *data, const unsigned short data_size);
    bool isRunning(void);
    bool filterIsEnabled(void);

private slots:
    void on_startButton_clicked();

    void on_stopButton_clicked();

    void on_opendirButton_clicked();

    void keyPressEvent(QKeyEvent *e);

private:
    Ui::DirectDiskDialog *ui;
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void directdisk_stop(void);

    QSemaphore sem;
    QString savefile_path;
    bool is_running;
    qint64 directdisk_filelen;
    QFile* directdisk_file;
    bool filter;
};

#endif // DIRECTDISKDIALOG_H
