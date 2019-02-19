#include "terminaldialog.h"
#include "ui_terminaldialog.h"
#include "history_line_edit.hpp"

#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QScrollBar>
#include <QTextStream>
#include <QDockWidget>
#include <QtSerialPort>
#include <QtSerialPort/QSerialPort>
#include <QSettings>
#include <QProgressDialog>

#include "sniff/sniff.h"

#define SETTINGS_FILENAME "hydratool.ini"

TerminalDialog::TerminalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TerminalDialog),
    sem(1),
    serialport_timer_sem(1)
{
    /*Variables init  */
    serialport_nb_packets_rx = 0;
    serialport_nb_bytes_rx = 0;
    serialport_nb_bytes_tx = 0;
    itsOkToClose = false;
    HexAscii = false;
    AutoScrollEnabled = false;
    PCdateTimeEnabled = false;
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    /* SerialPort / Settings configuration */
    serial = new QSerialPort(this);
    serialinfo = new QSerialPortInfo();
    settingsCom = new SettingsDialog;
    directdisk = new DirectDiskDialog;
    directdisk->setWindowTitle("Terminal Save Direct To Disk");

    ui->console->setEnabled(true);
    ui->console->setWordWrapMode(QTextOption::NoWrap);
    ui->console->setUndoRedoEnabled(true);
    ui->console->setLocalVerticalScrollBarMaxEnabled(true);

    readSettings();

    /* Force ReadOnly */
    ui->console->setReadOnly(true);
    ui->console->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(serial, SIGNAL(readyRead()), this, SLOT(rxData()));
    connect(settingsCom, SIGNAL(settings_hide(bool)), this, SLOT(settings_hide(bool)));
    connect(&serialport_timer, SIGNAL(timeout()), this, SLOT(serialport_timer_update()));
}

TerminalDialog::~TerminalDialog()
{
    writeSettings();
    delete serial;
    delete serialinfo;
    delete settingsCom;
    delete directdisk;
    delete ui;
}

void TerminalDialog::readSettings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);
    SettingsDialog::Settings p;

    settings.beginGroup("terminaldialog");
    ui->actionConnect->setChecked(settings.value("COMPortConnected", "false").toBool());

    p.name = settings.value("COMPortName", "").toString();
    p.baudRate = settings.value("COMPortBaudRate", "115200").toInt();
    p.stringBaudRate = QString::number(p.baudRate);

    p.dataBits = static_cast<QSerialPort::DataBits>(settings.value("COMPortDataBits", "8").toInt());
    QMetaEnum eDataBits = QMetaEnum::fromType<QSerialPort::DataBits>();
    p.stringDataBits = eDataBits.valueToKey(p.dataBits);

    p.parity = static_cast<QSerialPort::Parity>(settings.value("COMPortParity", "0").toInt());
    QMetaEnum eParity = QMetaEnum::fromType<QSerialPort::Parity>();
    p.stringParity = eParity.valueToKey(p.parity);

    p.stopBits = static_cast<QSerialPort::StopBits>(settings.value("COMPortStopBits", "1").toInt());
    QMetaEnum eStopBits = QMetaEnum::fromType<QSerialPort::StopBits>();
    p.stringStopBits = eStopBits.valueToKey(p.stopBits);

    p.flowControl = static_cast<QSerialPort::FlowControl>(settings.value("COMPortFlowControl", "0").toInt());
    QMetaEnum eflowControl = QMetaEnum::fromType<QSerialPort::FlowControl>();
    p.stringFlowControl = eflowControl.valueToKey(p.flowControl);

    settingsCom->set_settings(&p);

    if(ui->actionConnect->isChecked() == true)
    {
        openSerialPort();
    }
    settings.endGroup();
}

void TerminalDialog::writeSettings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);

    settings.beginGroup("terminaldialog");

    settings.setValue("COMPortConnected", ui->actionConnect->isChecked());

    if(ui->actionConnect->isChecked() == true)
    {
        SettingsDialog::Settings p = settingsCom->get_settings();

        settings.setValue("COMPortName", p.name);
        settings.setValue("COMPortBaudRate", p.baudRate);
        settings.setValue("COMPortDataBits", p.dataBits);
        settings.setValue("COMPortParity", p.parity);
        settings.setValue("COMPortStopBits", p.stopBits);
        settings.setValue("COMPortFlowControl", p.flowControl);
    }

    settings.endGroup();
}

void TerminalDialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);

    serialport_timer_sem.acquire();
    serialport_timer.stop();
    serialport_timer_sem.release();

    serial->close();
    settingsCom->close();
    directdisk->close();
    itsOkToClose = true;
}

void TerminalDialog::reject()
{
    if (itsOkToClose)
    {
        QDialog::reject();
    }
}

bool TerminalDialog::readBase(const char * str, int * value, int base, int size)
{
    char current;
    int i = 0;
    int temp = 0;
    char max = '0' + char(base) - 1;

    *value = 0;

    while(i < size) {
        current = str[i];
        if (current >= 'A')
        {
            current &= 0xdf;
            current -= ('A' - '0' - 10);
        }
        else if (current > '9')
            return false;

        if (current >= '0' && current <= max) {
            temp *= base;
            temp += (current - '0');
        } else {
            return false;
        }
        i++;
    }
    *value = temp;
    return true;
}

int TerminalDialog::convExtendedToString(const char* str_in, char* str_out, int str_in_len)
{
    int str_in_idx = 0;
    int str_out_idx = 0;
    int charLeft = str_in_len;
    char current;
    while (str_in_idx < str_in_len)
    {
         current = str_in[str_in_idx];
        --charLeft;
        if (current == '\\' && charLeft)
        {
            //possible escape sequence
            str_in_idx++;
            charLeft--;
            current = str_in[str_in_idx];
            switch(current)
            {
                case 'r':
                    str_out[str_out_idx] = '\r';
                    break;
                case 'n':
                    str_out[str_out_idx] = '\n';
                    break;
                case '0':
                    str_out[str_out_idx] = '\0';
                    break;
                case 't':
                    str_out[str_out_idx] = '\t';
                    break;
                case '\\':
                    str_out[str_out_idx] = '\\';
                    break;
                case 'b':
                case 'd':
                case 'o':
                case 'x':
                case 'u':
                {
                    int size = 0, base = 0;
                    if (current == 'b')
                    {	//11111111
                        size = 8, base = 2;
                    }
                    else if (current == 'o')
                    {	//377
                        size = 3, base = 8;
                    }
                    else if (current == 'd')
                    {	//255
                        size = 3, base = 10;
                    }
                    else if (current == 'x')
                    {	//0xFF
                        size = 2, base = 16;
                    }
                    else if (current == 'u')
                    {	//0xCDCD
                        size = 4, base = 16;
                    }

                    if (charLeft >= size)
                    {
                        int res = 0;
                        if (readBase(str_in+(str_in_idx+1), &res, base, size))
                        {
                            str_out[str_out_idx] = (char)res;
                            str_in_idx += size;
                            break;
                        }
                    }
                    //not enough chars to make parameter, use default method as fallback
                }

                default:
                {	//unknown sequence, treat as regular text
                    str_out[str_out_idx] = '\\';
                    str_out_idx++;
                    str_out[str_out_idx] = current;
                    break;
                }
            }
        }
        else
        {
            str_out[str_out_idx] = str_in[str_in_idx];
        }
        str_in_idx++;
        str_out_idx++;
    }
    str_out[str_out_idx] = 0;
    return str_out_idx;
}

void TerminalDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}

void TerminalDialog::clear(void)
{
    ui->console->clear();
}

Qt::CheckState TerminalDialog::PCdateTimeCheckState(void)
{
    return ui->checkBoxPCdateTime->checkState();
}

void TerminalDialog::setPCdateTimeCheckState(Qt::CheckState checked)
{
    ui->checkBoxPCdateTime->setCheckState(checked);
    if(checked == Qt::Checked)
        on_checkBoxPCdateTime_clicked(true);
    if(checked == Qt::Unchecked)
        on_checkBoxPCdateTime_clicked(false);
}

void TerminalDialog::on_clearButton_clicked()
{
    ui->console->clear();
}

void TerminalDialog::on_lineEdit_lineExecuted(QString text)
{
    this->lineEdit_text = text;
}

void TerminalDialog::on_lineEdit_returnPressed()
{
    QString text_data;

    text_data = this->lineEdit_text;
    if(this->ui->checkBox_CR->isChecked())
        text_data += '\x0D';

    if(this->ui->checkBox_LF->isChecked())
        text_data += '\x0A';

    txData(text_data);

    ui->lineEdit->clear();
}

void TerminalDialog::on_saveButton_clicked()
{
    if(this->savefile_path.length() == 0)
    {
        this->savefile_path = "./";
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save file"), this->savefile_path, "Text files (*.txt);;All files (*.*)");
    if (fileName != "")
    {
        this->savefile_path = QFileInfo(fileName).path(); // store path for next time
        QFile file(fileName);

        if (file.open(QIODevice::ReadWrite))
        {
            QTextStream stream(&file);
            stream << ui->console->toPlainText();
            file.flush();
            file.close();
        }
        else
        {
            QMessageBox::critical(this, tr("Error"), tr("Error to save file"));
            return;
        }
    }
}

void TerminalDialog::on_checkBoxPCdateTime_clicked(bool checked)
{
    PCdateTimeEnabled = checked;
}

void TerminalDialog::on_ConfigureButton_clicked()
{
    settingsCom->show();
}

void TerminalDialog::serialport_timer_update()
{
    QString label_com_str;
    bool com_port_found = false;
    qint64 nb_bytes = 0;
/*
QTime timer;
int elapsed_ms;
QString timing;
timer.start();
*/
    QSerialPortInfo portinfo(serial->portName());
    if(portinfo.isNull() == false)
    {
        if( (portinfo.hasVendorIdentifier() == serialinfo->hasVendorIdentifier()) &&
            (portinfo.hasProductIdentifier() == serialinfo->hasProductIdentifier())
           )
            com_port_found = true;
    }
/*
elapsed_ms = timer.elapsed();
timing = "availablePorts scan time = " + QString::number(elapsed_ms) + " ms";
ui->console->putData(timing.toLocal8Bit());
*/
    if(com_port_found == false)
    {
        sem.acquire();
        if (serial->isOpen())
        {
            ui->actionConnect->setChecked(false);
            serial->close();
            ui->label_com->setText("");
            ui->statusBar->setText("Disconnected");
            /* Change timer to fast scan */
            serialport_timer.start(200);
        }
        sem.release();
    }else
    {
        sem.acquire();
        if (serial->isOpen() == false)
        {
            sem.release();
            openSerialPort();
        }else
        {
            nb_bytes = serial->bytesAvailable();
            sem.release();
        }
        serialport_timer_sem.acquire();
        label_com_str = "RX " + QString::number((float)(serialport_nb_bytes_rx)/1024.0f, 'f', 2) + " KB/s " + QString::number(serialport_nb_packets_rx) + " fps";
        ui->label_com->setText(label_com_str);

        serialport_nb_bytes_rx = 0;
        serialport_nb_packets_rx = 0;
        serialport_timer_sem.release();
    }
    if(nb_bytes > 0)
    {
        rxData();
    }
}

void TerminalDialog::txData(const QString &text_data)
{
    #define BYTE_SIZE (1024)
    char bytes[BYTE_SIZE+1];
    int text_data_len;
    int nb_bytes;

    text_data_len = text_data.length();
    if(text_data_len > BYTE_SIZE)
        text_data_len = BYTE_SIZE;

    nb_bytes = convExtendedToString(text_data.toStdString().c_str(), bytes, text_data_len);

    sem.acquire();
    if (serial->isOpen())
    {
        serial->write(bytes, nb_bytes);
    }else
    {
        this->displayData("Error COM port not connected\n");
    }
    sem.release();
}

void TerminalDialog::raw_data()
{
#if 0
    unsigned long nb_bytes;
    qint64 nb_byte_available;
    QByteArray data;
    QByteArray qb_data;

    if(PCdateTimeEnabled == true)
    {
        QDateTime local(QDateTime::currentDateTime());
        QString datehour_ms = local.toString("dd/MM/yy hh:mm:ss zzz") + "ms ";
        qb_data.append(datehour_ms);
    }

    nb_byte_available = serial->bytesAvailable();
    data = serial->read(nb_byte_available);

    //directdisk->putData(&sniff_frame);

    if(AsciiEnabled == false && HexEnabled == false)
    {
        qb_data += data;
    }else
    {
        if(AsciiEnabled == true)
        {
            /* Print Data as ASCII */
            bin_to_ascii_txt(data.si, data.size, (t_u8*)temp_string);
            qb_data.append(temp_string);
            qb_data.append(' ');
        }

        if(HexEnabled == true)
        {
            /* Print Data as Hex Text */
            t_ascii_hex val_ascii;
            for (int i = 0; i < sniff_frame.data_size; i++)
            {
                val_ascii = BinHextoAscii(sniff_frame.data[i]);
                qb_data.append(val_ascii.msb);
                qb_data.append(val_ascii.lsb);
                qb_data.append(' ');
            }
            //qb_data.append(' ');
        }
    }

    //qb_data.append('\n');
    displayData(qb_data);
#endif
}

static t_u32 bin_to_ascii_txt(t_u8* in_buffer, t_u16 in_buffer_size, t_u8* out_buffer)
{
    t_u8* pt_src;
    t_u32 src_len;
    t_u8* pt_dst;
    t_u32 i;

    src_len = in_buffer_size;
    pt_src = in_buffer;
    pt_dst = out_buffer;

    for (i = 0; i < src_len; i++)
    {
        *pt_dst = ascii_table[pt_src[i]];
        pt_dst++;
    }

    /* Add Null string at end */
    *pt_dst = 0;
    pt_dst++;

    return (pt_dst - out_buffer);
}

void TerminalDialog::rxData()
{
    qint64 nb_bytes;
    QByteArray qb_data;

    sem.acquire();
    if (serial->isOpen())
    {
        nb_bytes = serial->bytesAvailable();

        qb_data = serial->read(nb_bytes);
        directdisk->putData(qb_data.data(),qb_data.size());

        if(HexAscii == false)
        {
            displayData(qb_data);
        }else
        {
            /* Print Data as Hex & Ascii */
            QByteArray qb_dataHexAscii;
            #define HEXASCII_NBDATA (24)
            char* data;
            t_ascii_hex val_ascii;
            char ascii[HEXASCII_NBDATA+1];
            ascii[HEXASCII_NBDATA] = '\0';
            data = qb_data.data();
            for (int i = 0; i < qb_data.size(); i++)
            {
                val_ascii = BinHextoAscii(data[i]);
                qb_dataHexAscii.append(val_ascii.msb);
                qb_dataHexAscii.append(val_ascii.lsb);
                qb_dataHexAscii.append(' ');

                if (data[i] >= 0x20 && data[i] <= 0x7f) {
                    ascii[i % HEXASCII_NBDATA] = data[i];
                } else {
                    ascii[i % HEXASCII_NBDATA] = '.';
                }

                if ((i+1) % HEXASCII_NBDATA == 0)
                {
                    qb_dataHexAscii += "| ";
                    qb_dataHexAscii += ascii;
                    qb_dataHexAscii += "\r\n";
                    displayData(qb_dataHexAscii);
                    qb_dataHexAscii.clear();
                } else if (i+1 == qb_data.size())
                {
                    ascii[(i+1) % HEXASCII_NBDATA] = '\0';
                    for (int j = (i+1) % HEXASCII_NBDATA; j < HEXASCII_NBDATA; ++j) {
                        qb_dataHexAscii.append("   ");
                    }
                    qb_dataHexAscii += "| ";
                    qb_dataHexAscii += ascii;
                    qb_dataHexAscii += "\r\n";
                    displayData(qb_dataHexAscii);
                    qb_dataHexAscii.clear();
                }
            }
        }

        serialport_timer_sem.acquire();
        serialport_nb_bytes_rx += nb_bytes - serial->bytesAvailable();
        serialport_nb_packets_rx += 1;
        serialport_timer_sem.release();
    }
    sem.release();
#if 0
    sem.acquire();
    if (serial->isOpen())
    {
        nb_bytes = serial->bytesAvailable();

        if(terminal_chan->isVisible())
        {
            QByteArray qb_data;
            qb_data = serial->read(nb_bytes);
            directdisk->putData(qb_data.constData(), nb_bytes);
            /*
            qb_data.replace(QByteArray("\r\n"),QByteArray("\n"));
            qb_data.replace(QByteArray("\r"),QByteArray(""));
            */
            terminal_chan->putData(qb_data);
        }else
        {
            //snprintf(tmp_string, TMP_STRING_LEN, "nb bytes %ld time ms %d", (long)serial->bytesAvailable(), t.elapsed());
            //ui->statusBar->showMessage(tmp_string);
            if(RawEnabled == true)
            {
                raw_data();
            }else
            {
                sniff_trace(serial, 0);
            }
        }

        serialport_timer_sem.acquire();
        serialport_nb_bytes_rx += nb_bytes - serial->bytesAvailable();
        serialport_nb_packets_rx += 1;
        serialport_timer_sem.release();
    }
    sem.release();
#endif
}

void TerminalDialog::displayData(const QString &TextLine)
{
    QChar text_char;
    bool newline;
    QScrollBar *bar = ui->console->verticalScrollBar();
    ui->console->moveCursor(QTextCursor::End);

    newline = false;
    for(int i=0; i<TextLine.length(); i++)
    {
        text_char = TextLine.at(i);
        if(text_char == '\n')
            newline = true;

        FullTextLine += text_char;
        if(newline == true)
        {
            if(PCdateTimeEnabled == true)
            {
                QDateTime local(QDateTime::currentDateTime());
                QString datehour_ms = local.toString("dd/MM/yy hh:mm:ss zzz") + "ms ";
                ui->console->insertPlainText(datehour_ms + FullTextLine);
            }else
            {
                ui->console->insertPlainText(FullTextLine);
            }
            bar->setValue(bar->maximum());

            newline = false;
            FullTextLine = "";
        }
    }

    if(FullTextLine.size() > 0)
    {
        ui->console->insertPlainText(FullTextLine);
        bar->setValue(bar->maximum());
        FullTextLine = "";
    }
}

void TerminalDialog::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        ui->statusBar->setText("QSerialPort::ResourceError " + serial->errorString());
        if (serial->isOpen())
        {
            closeSerialPort();
            openSerialPort();
        }
    }
}

void TerminalDialog::on_actionConnect_clicked(bool checked)
{
    if(checked == true)
    {
        openSerialPort();
    }else
    {
        closeSerialPort();
    }
}

void TerminalDialog::settings_hide(bool change_settings)
{
    if(change_settings == true)
    {
        if (serial->isOpen() == false)
        {
            openSerialPort();
        }else
        {
            closeSerialPort();
            openSerialPort();
        }
    }
}

#define SERIAL_RX_BUFFER (128 * 1024 * 1024)
void TerminalDialog::openSerialPort()
{
    serialport_timer_sem.acquire();
    serialport_timer.stop();
    serialport_nb_bytes_rx = 0;
    serialport_nb_bytes_tx = 0;
    serialport_nb_packets_rx = 0;
    serialport_timer_sem.release();

    sem.acquire();

    SettingsDialog::Settings p = settingsCom->get_settings();

    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);

    if (serial->open(QIODevice::ReadWrite)) {
            delete serialinfo;
            serialinfo = new QSerialPortInfo(p.name);
            //serial->setReadBufferSize(SERIAL_RX_BUFFER); /* By default set to 0 = No Limit */

            //ui->console->setReadOnly(true);
            //console->setTextInteractionFlags(Qt::NoTextInteraction);
            //ui->console->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

            //console->setEnabled(true);
            //ui->console->setLocalEchoEnabled(p.localEchoEnabled);
            ui->console->setLocalEchoEnabled(false);
            ui->console->setLocalVerticalScrollBarMaxEnabled(AutoScrollEnabled);
            ui->actionConnect->setChecked(true);
            //ui->actionConnect->setEnabled(false);
            //ui->actionDisconnect->setEnabled(true);
            ui->statusBar->setText(tr("Connected to %1 : %2, %3, %4, %5, %6")
                                       .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                       .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
            //serial->waitForReadyRead(100);
    } else {
        ui->actionConnect->setChecked(false);
        ui->statusBar->setText(tr("Error to connect to %1 : %2, %3, %4, %5, %6")
                                   .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                   .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    }
    sem.release();

    serialport_timer_sem.acquire();
    serialport_timer.start(1000);
    serialport_timer_sem.release();
}

void TerminalDialog::closeSerialPort()
{
    serialport_timer_sem.acquire();
    serialport_timer.stop();
    //ui->label_com->setText("");
    serialport_nb_bytes_rx = 0;
    serialport_nb_packets_rx = 0;
    serialport_timer_sem.release();

    sem.acquire();
    if (serial->isOpen())
    {
        ui->actionConnect->setChecked(false);
        serial->close();
    }
    //ui->console->setReadOnly(false);
    ui->statusBar->setText(tr("Disconnected"));
    sem.release();
}

void TerminalDialog::on_saveDirectToDisk_clicked()
{
    //dockwidget_directdisk_ext->show();
    directdisk->raise();
    directdisk->show();
}

void TerminalDialog::on_checkBoxHexAsc_clicked(bool checked)
{
    HexAscii = checked;
}

void TerminalDialog::on_sendFileButton_clicked()
{
    int line_delay_ms;
    QString fileName;
    QFile file;
    QString text_data;
    qint64 lineLength;
    char text_buf[512+1];

    line_delay_ms = ui->SendFileSpinBox->value();
    if(this->loadfile_path.length() == 0)
    {
        this->loadfile_path = "./";
    }

    fileName = QFileDialog::getOpenFileName(this, "Select text file to send...", this->loadfile_path, "text files (*.txt);;All files (*.*)");
    if(!fileName.length()) return;

    this->loadfile_path = QFileInfo(fileName).path(); // store path for next time

    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);

    if(QFileInfo(fileName).suffix().toLower() == "txt")
    {
        QProgressDialog progress("SendFile in progress...", "Cancel", 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setRange(0,0);
        progress.setMinimumDuration(0);
        progress.show();

        while(1)
        {
            lineLength = file.readLine(text_buf, 512);
            if (lineLength != -1)
            {
                // the line is available in buf
                text_data = text_buf;
                txData(text_data);
                qApp->processEvents(QEventLoop::AllEvents, 1);
                QThread::msleep(line_delay_ms - 1);

                if (progress.wasCanceled())
                {
                    break;
                }
            }else
            {
                break; /* End Of File */
            }
        }
        progress.cancel();
    }
    file.close();
}
