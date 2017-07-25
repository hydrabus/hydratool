#include "directdiskdialog.h"
#include "ui_directdiskdialog.h"

#include <QString>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QKeyEvent>

DirectDiskDialog::DirectDiskDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DirectDiskDialog),
    sem(1),
    is_running(false),
    directdisk_file(NULL)
{
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
}

DirectDiskDialog::~DirectDiskDialog()
{
    delete ui;
}

void DirectDiskDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}

void DirectDiskDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void DirectDiskDialog::hideEvent(QHideEvent *event)
{
    directdisk_stop();
    QWidget::hideEvent(event);
    //QMessageBox::critical(this, tr("Stop & Close"), tr("Stop & Close file"));
}

void DirectDiskDialog::directdisk_stop(void)
{
    sem.acquire();
    if(is_running == true)
    {
        directdisk_file->close();
        if(directdisk_filelen == 0)
            directdisk_file->remove();
        delete directdisk_file;
        directdisk_file = NULL;
        is_running = false;
        this->ui->status->setText("Direct Disk Stopped");
   }
   sem.release();
}

void DirectDiskDialog::putData(const t_sniff_frame *in_frame)
{
    qint64 data_written;

    sem.acquire();
    if(is_running == true)
    {
        data_written = directdisk_file->write((char*)in_frame->data_hdr, in_frame->data_hdr_size);
        if(data_written > 0)
            directdisk_filelen += data_written;

        data_written = directdisk_file->write((char*)in_frame->data, in_frame->data_size);
        if(data_written > 0)
            directdisk_filelen += data_written;

        //directdisk_file->flush();

        this->ui->filesize->setText(
                    QString::number(directdisk_filelen) + " Bytes / " +
                    QString::number( (float)directdisk_filelen / 1024.0f, 'f', 3) + " KBytes / " +
                    QString::number( (float)directdisk_filelen / (float)(1024*1024), 'f', 3) + " MBytes");
    }
    sem.release();
}

void DirectDiskDialog::putData(const char *data, const unsigned short data_size)
{
    qint64 data_written;

    sem.acquire();
    if(is_running == true)
    {
        data_written = directdisk_file->write(data, data_size);
        if(data_written > 0)
            directdisk_filelen += data_written;

        directdisk_file->flush();

        this->ui->filesize->setText(
                    QString::number(directdisk_filelen) + " Bytes / " +
                    QString::number( (float)directdisk_filelen / 1024.0f, 'f', 3) + " KBytes / " +
                    QString::number( (float)directdisk_filelen / (float)(1024*1024), 'f', 3) + " MBytes");
    }
    sem.release();
}

bool DirectDiskDialog::isRunning(void)
{
    bool local_is_running;

    sem.acquire();
    local_is_running = is_running;
    sem.release();
    return local_is_running;
}

void DirectDiskDialog::on_startButton_clicked()
{
    QString directdisk_filename;

    directdisk_stop();

    if(this->savefile_path.length() == 0)
    {
        this->savefile_path = "./";
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"), this->savefile_path, "Raw bin files (*.bin);;All files (*.*)");

    if( fileName == "" )
    {
        QDateTime local(QDateTime::currentDateTime());
        QString datehour = local.toString("yyyy_MM_dd_hh_mm_ss");
        fileName.append("./" + datehour + ".bin");

        QFile file(fileName);
        if( file.exists() == true)
        {
            QDateTime local(QDateTime::currentDateTime());
            QString datehour = local.toString("yyyy_MM_dd_hh_mm_ss");
            fileName.append("./" + datehour + ".bin");
        }
        file.close();
    }

    sem.acquire();
    /*
    QFile file(fileName);
    if( file.exists() == true)
    {
        QDateTime local(QDateTime::currentDateTime());
        QString datehour = local.toString("MMddhhmmss");
        fileName.append("./" + datehour + ".trc");
    }
    file.close();
    */

    directdisk_filename = fileName;
    directdisk_file = new QFile(directdisk_filename);
    if (directdisk_file->open(QIODevice::WriteOnly))
    {
        this->savefile_path = QFileInfo(fileName).path(); // store path for next time
        directdisk_filelen = 0;
        this->ui->filepath->setText(directdisk_filename);
        this->ui->filesize->setText(
                    QString::number(directdisk_filelen) + " Bytes / " +
                    QString::number( (float)directdisk_filelen / 1024.0f, 'f', 3) + " KBytes / " +
                    QString::number( (float)directdisk_filelen / (float)(1024*1024), 'f', 3) + " MBytes");
        this->ui->status->setText("Direct Disk Started");
        is_running = true;
    }else
    {
        this->ui->status->setText("Error to open file");
        is_running = false;
    }
    sem.release();
    this->update();
}

void DirectDiskDialog::on_stopButton_clicked()
{
    directdisk_stop();
}

void DirectDiskDialog::on_opendirButton_clicked()
{
    QString path = QDir::toNativeSeparators(this->savefile_path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
