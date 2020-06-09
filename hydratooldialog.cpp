#include <QFileDialog>
#include <QMainWindow>

#include "hydratooldialog.h"
#include "ui_hydratooldialog.h"

#include "progress.h"

#include <QMessageBox>
#include <QtSerialPort>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QTime>
#include <QThread>
#include <QBuffer>
#include <QTextStream>
#include <QSettings>

namespace Ui {
  class hydratooldialog;
}

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf(buf,len, format,...) _snprintf_s(buf, len,len, format, __VA_ARGS__)
#endif

hydratooldialog::hydratooldialog(QWidget *parent,
                                 QDockWidget *dockwidget_hydratool_directdisk,
                                 QDockWidget *dockwidget_terminal_directdisk,
                                 QDockWidget *dockwidget_terminal,
                                 QDockWidget *dockwidget_find) :
    QDialog(parent),
    ui(new Ui::hydratooldialog),
    sem(1),
    serialport_timer_sem(1)
{
    QFont font;
    console_find = new QPlainTextEdit;
    console_find->setFocusPolicy(Qt::ClickFocus);

    /*Variables init  */
    findLiveRefresh = false;
    serialport_nb_packets_rx = 0;
    serialport_nb_bytes_rx = 0;
    serialport_nb_bytes_tx = 0;
    itsOkToClose = false;

    /* dockWidget Init */
    dockwidget_hydratool_directdisk_ext = dockwidget_hydratool_directdisk;
    dockwidget_terminal_directdisk_ext = dockwidget_terminal_directdisk;
    dockwidget_terminal_ext = dockwidget_terminal;

    dockwidget_hydratool_directdisk_ext->hide();
    dockwidget_terminal_directdisk_ext->hide();
    dockwidget_terminal_ext->hide();

    ui->setupUi(this);

    on_historyDepthLines_returnPressed();

    ui->label_title->setText("HydraNFC real-time sniffer <a href=\"https://github.com/hydrabus/hydrafw/wiki/HydraFW-HydraNFC-v1-guide#sniffer-iso14443a-with-unique-hard-real-time-infinite-trace-mode\">HW Setup Link</a>");
    ui->label_title->setTextFormat(Qt::RichText);
    ui->label_title->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label_title->setOpenExternalLinks(true);

    ui->findPredefBox->addItem(QStringLiteral(""), "");
    ui->findPredefBox->addItem(QStringLiteral("REQA_WUPA"), "RDR 26|RDR 52");
    ui->findPredefBox->setCurrentIndex(0);

    /* SerialPort / Settings configuration */
    serial = new QSerialPort(this);
    serialinfo = new QSerialPortInfo();
    settingsCom = new SettingsDialog;

    terminal_chan = new TerminalDialog;
    terminal_chan->setWindowTitle("Terminal");

    directdisk = new DirectDiskDialog;
    directdisk->setWindowTitle("HydraNFC Save Direct To Disk");

    console = ui->console;
    console->setEnabled(true);
    console->setWordWrapMode(QTextOption::NoWrap);
    console->setUndoRedoEnabled(true);

    readSettings();

    RawEnabled = ui->actionRaw->isChecked();
    PCdateTimeEnabled = ui->actionPCdateTime->isChecked();
    AsciiEnabled = ui->actionAscii->isChecked();
    HexEnabled = ui->actionHex->isChecked();
    AutoScrollEnabled = false;

    if(PCdateTimeEnabled == true)
    {
        terminal_chan->setPCdateTimeCheckState(Qt::Checked);
    }else
    {
        terminal_chan->setPCdateTimeCheckState(Qt::Unchecked);
    }

    ui->actionConnect->setEnabled(true);

    initActionsConnections();

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(settingsCom, SIGNAL(settings_hide(bool)), this, SLOT(settings_hide(bool)));

    connect(terminal_chan, SIGNAL(getData(const QString)), this, SLOT(terminal_getData(const QString)));

    connect(&serialport_timer, SIGNAL(timeout()), this, SLOT(serialport_timer_update()));

    /* External dockWidget configuration */
    dockwidget_find_ext = dockwidget_find;
    font.setFamily(QStringLiteral("Courier New"));
    console_find->setFont(font);
    console_find->setLineWrapMode(QPlainTextEdit::NoWrap);
    dockwidget_find_ext->setWindowTitle("HydraNFC Find result");
    dockwidget_find_ext->setWidget(console_find);

    dockwidget_terminal_ext->setWindowTitle(terminal_chan->windowTitle());
    dockwidget_terminal_ext->setWidget(terminal_chan);

    dockwidget_hydratool_directdisk_ext->setWindowTitle(directdisk->windowTitle());
    dockwidget_hydratool_directdisk_ext->setWidget(directdisk);
}

hydratooldialog::~hydratooldialog()
{
    writeSettings();
    delete serial;
    delete serialinfo;
    delete settingsCom;
    delete terminal_chan;
    delete directdisk;
    delete console_find;
    delete ui;
}

void hydratooldialog::readSettings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);

    settings.beginGroup("hydratooldialog");
    ui->actionRaw->setChecked(settings.value("RawEnabled", "false").toBool());
    ui->actionPCdateTime->setChecked(settings.value("PCdateTimeEnabled", "false").toBool());
    ui->actionAscii->setChecked(settings.value("AsciiEnabled", "false").toBool());
    ui->actionHex->setChecked(settings.value("HexEnabled", "false").toBool());
    ui->actionConnect->setChecked(settings.value("COMPortConnected", "false").toBool());

    SettingsDialog::Settings p; // = settingsCom->get_settings();
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
        on_actionConnectDisconnect();
    }
    settings.endGroup();
}

void hydratooldialog::writeSettings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);
    SettingsDialog::Settings p = settingsCom->get_settings();

    settings.beginGroup("hydratooldialog");
    settings.setValue("RawEnabled", ui->actionRaw->isChecked());
    settings.setValue("PCdateTimeEnabled", ui->actionPCdateTime->isChecked());
    settings.setValue("AsciiEnabled", ui->actionAscii->isChecked());
    settings.setValue("HexEnabled", ui->actionHex->isChecked());

    settings.setValue("COMPortConnected", ui->actionConnect->isChecked());

    if(ui->actionConnect->isChecked() == true)
    {
        settings.setValue("COMPortName", p.name);
        settings.setValue("COMPortBaudRate", p.baudRate);
        settings.setValue("COMPortDataBits", p.dataBits);
        settings.setValue("COMPortParity", p.parity);
        settings.setValue("COMPortStopBits", p.stopBits);
        settings.setValue("COMPortFlowControl", p.flowControl);
    }
    settings.endGroup();
}

void hydratooldialog::on_actionConnectDisconnect()
{
    if(ui->actionConnect->isChecked() == true)
    {
        openSerialPort();
    }else
    {
        closeSerialPort();
    }
}

void hydratooldialog::settings_hide(bool change_settings)
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

void hydratooldialog::serialport_timer_update()
{
    QString label_com_str;
    bool com_port_found = false;
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
        if (serial->isOpen())
        {
            ui->actionConnect->setChecked(false);
            serial->close();
            ui->label_com->setText("");
            ui->statusBar->showMessage("Disconnected");
            /* Change timer to fast scan */
            serialport_timer.start(200);
        }
    }else
    {
        if (serial->isOpen() == false)
        {
            openSerialPort();
        }

        serialport_timer_sem.acquire();
        label_com_str = "RX " + QString::number((float)(serialport_nb_bytes_rx)/1024.0f, 'f', 2) + " KB/s " + QString::number(serialport_nb_packets_rx) + " fps";
        ui->label_com->setText(label_com_str);

        serialport_nb_bytes_rx = 0;
        serialport_nb_packets_rx = 0;
        serialport_timer_sem.release();
    }
}

void hydratooldialog::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);

    serialport_timer_sem.acquire();
    serialport_timer.stop();
    serialport_timer_sem.release();

    serial->close();
    settingsCom->close();
    terminal_chan->close();
    directdisk->close();
    itsOkToClose = true;
}

void hydratooldialog::reject()
{
    if (itsOkToClose)
    {
        QDialog::reject();
    }
}

void hydratooldialog::on_historyDepthLines_returnPressed()
{
    int nb_lines;
    QString historyDepthLines;
    historyDepthLines = ui->historyDepthLines->text();

    nb_lines = historyDepthLines.toInt();
    if(nb_lines < 1)
    {
        ui->console->document()->setMaximumBlockCount(-1); /* No Limit */
        ui->statusBar->showMessage(tr("History depth no limit"));
    }else
    {
        ui->console->document()->setMaximumBlockCount(nb_lines);
        ui->statusBar->showMessage(tr("History depth %1 lines").arg(nb_lines));
    }
}

void hydratooldialog::terminal_getData(const QString &text_data)
{
    QByteArray bytes = text_data.toLocal8Bit();

    sem.acquire();
    if (serial->isOpen())
    {
        serial->write(bytes, bytes.length());
    }else
    {
        terminal_chan->displayData("Error COM port not connected\n");
    }
    sem.release();
}

#define SERIAL_RX_BUFFER (128 * 1024 * 1024)
void hydratooldialog::openSerialPort()
{
    serialport_timer_sem.acquire();
    serialport_timer.stop();
    serialport_nb_bytes_rx = 0;
    serialport_nb_bytes_tx = 0;
    serialport_nb_packets_rx = 0;
    serialport_timer_sem.release();

    sem.acquire();
    sniff_frame_state = SNIFF_FRAME_INIT_DECODE;
    memset(&sniff_frame, 0, sizeof(sniff_frame));

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

            //console->setReadOnly(false);
            //console->setTextInteractionFlags(Qt::NoTextInteraction);
            //console->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

            //console->setEnabled(true);
            console->setLocalVerticalScrollBarMaxEnabled(AutoScrollEnabled);
            ui->actionConnect->setChecked(true);
            //ui->actionConnect->setEnabled(false);
            //ui->actionDisconnect->setEnabled(true);
            ui->statusBar->showMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                                       .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                       .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
            //serial->waitForReadyRead(100);
    } else {
        ui->actionConnect->setChecked(false);
        ui->statusBar->showMessage(tr("Error to connect to %1 : %2, %3, %4, %5, %6")
                                   .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                   .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    }
    sem.release();

    serialport_timer_sem.acquire();
    serialport_timer.start(1000);
    serialport_timer_sem.release();
}

void hydratooldialog::closeSerialPort()
{
    serialport_timer_sem.acquire();
    serialport_timer.stop();
    ui->label_com->setText("");
    serialport_nb_bytes_rx = 0;
    serialport_nb_packets_rx = 0;
    serialport_timer_sem.release();

    sem.acquire();
    if (serial->isOpen())
    {
        ui->actionConnect->setChecked(false);
        serial->close();
    }
    //console->setReadOnly(false);
    //ui->actionConnect->setEnabled(true);
    //ui->actionDisconnect->setEnabled(false);
    ui->statusBar->showMessage(tr("Disconnected"));
    sem.release();
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

void hydratooldialog::raw_data(void)
{
    qint64 nb_byte_available;
    QByteArray qb_data;

    if(PCdateTimeEnabled == true)
    {
        QDateTime local(QDateTime::currentDateTime());
        QString datehour_ms = local.toString("dd/MM/yy hh:mm:ss zzz") + "ms ";
        qb_data.append(datehour_ms);
    }

    nb_byte_available = serial->bytesAvailable();
    sniff_frame.data_size = nb_byte_available;
    serial->read((char*)sniff_frame.data, nb_byte_available);

    directdisk->putData(&sniff_frame);

    if(AsciiEnabled == false && HexEnabled == false)
    {
        qb_data.append((const char*)sniff_frame.data, (int)sniff_frame.data_size);
    }else
    {
        if(AsciiEnabled == true)
        {
            /* Print Data as ASCII */
            bin_to_ascii_txt(sniff_frame.data, sniff_frame.data_size, (t_u8*)temp_string);
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
    console->putData(qb_data);
}

/*
decode_nb_frames: if = 0 ignored decode maximum number of frame, if > 0 decode x frames or less and returns
*/
void hydratooldialog::sniff_trace(QIODevice* stream, int decode_nb_frames)
{
    qint64 nb_byte_read;
    qint64 nb_byte_available;
    int nb_frames = 0;

    nb_byte_available = stream->bytesAvailable();
    while(nb_byte_available > 0)
    {
        if(decode_nb_frames > 0)
        {
            if(nb_frames >= decode_nb_frames)
                break;

            nb_frames++;
        }

        if(sniff_frame_state == SNIFF_FRAME_INIT_DECODE)
        {
            if(nb_byte_available >= SNIFF_FRAME_HEADER_DECODE_NB_BYTES)
            {
                t_u8 data[SNIFF_FRAME_HEADER_DECODE_NB_BYTES];

                nb_byte_read = stream->read((char*)data, SNIFF_FRAME_HEADER_DECODE_NB_BYTES);
                if(nb_byte_read > 0)
                {
                    directdisk->putData((char*)data, nb_byte_read);
                }

                sniff_frame_state = sniff_frame_header_decode(data, &sniff_frame);
                if(sniff_frame_state == SNIFF_FRAME_INIT_DECODE)
                {
                    int nb_data;

                    qb_data.append('\n');
                    if(PCdateTimeEnabled == true)
                    {
                        QDateTime local(QDateTime::currentDateTime());
                        QString datehour_ms = local.toString("dd/MM/yy hh:mm:ss zzz") + "ms ";
                        qb_data.append(datehour_ms);
                    }
                    qb_data.append("Frame size error resync data:\n");
                    /* Print Data as Hex Text */
                    t_ascii_hex val_ascii;
                    nb_data = 0;
                    for (int i = 0; i < SNIFF_FRAME_HEADER_DECODE_NB_BYTES; i++)
                    {
                        val_ascii = BinHextoAscii(data[i]);
                        qb_data.append(val_ascii.msb);
                        qb_data.append(val_ascii.lsb);
                        qb_data.append(' ');
                        nb_data++;
                    }

                    /* Flush data until new frame*/
                    while(1)
                    {
                        nb_byte_read = stream->read((char*)data, 1);
                        if(nb_byte_read == 1)
                        {
                            directdisk->putData((char*)data, 1);
                        }
                        if(nb_byte_read == 0)
                        {
                            //QThread::usleep(10);
                            nb_byte_read = stream->read((char*)data, 1);
                            if(nb_byte_read == 1)
                            {
                                directdisk->putData((char*)data, 1);
                            }
                            if(nb_byte_read == 0)
                            {
                                break;
                            }
                        }
                        val_ascii = BinHextoAscii(data[0]);
                        qb_data.append(val_ascii.msb);
                        qb_data.append(val_ascii.lsb);
                        qb_data.append(' ');
                        nb_data++;

                        if(nb_data >= 64)
                        {
                            qb_data.append('\n');
                            nb_data = 0;
                        }
                    }

                    console->putData(qb_data);
                    qb_data.clear();
                }else
                {
                    qb_data.clear();
                }
                nb_byte_available = stream->bytesAvailable();
            }else
            {
                nb_byte_available = 0;
            }
        }

        if(sniff_frame_state == SNIFF_FRAME_START_TIMESTAMP_DECODE)
        {
            if(nb_byte_available >= SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES)
            {
                t_u8 data[SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES];

                nb_byte_read = stream->read((char*)data, SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES);
                nb_byte_available = stream->bytesAvailable();
                if(nb_byte_read > 0)
                {
                    directdisk->putData((char*)data, nb_byte_read);
                }
                sniff_frame_state = sniff_frame_timestamp_decode(data, &sniff_frame, true);
            }else
            {
                nb_byte_available = 0;
            }
        }

        if(sniff_frame_state == SNIFF_FRAME_DATA_DECODE)
        {
            t_u16 data_size;

            data_size = sniff_frame.data_size;
            if(nb_byte_available >= data_size)
            {
                nb_byte_read = stream->read((char*)sniff_frame.data, data_size);
                nb_byte_available = stream->bytesAvailable();
                if(nb_byte_read > 0)
                {
                    directdisk->putData((char*)sniff_frame.data, nb_byte_read);
                }

                if(PCdateTimeEnabled == true)
                {
                    QDateTime local(QDateTime::currentDateTime());
                    QString datehour_ms = local.toString("dd/MM/yy hh:mm:ss zzz") + "ms ";
                    qb_data.append(datehour_ms);
                }

                if(sniff_frame.protocol_options & PROTOCOL_OPTIONS_START_OF_FRAME_TIMESTAMP)
                {
                    snprintf(temp_string, TEMP_STRING_LEN, "%08X ", sniff_frame.start_of_frame_time.timestamp);
                    qb_data.append(temp_string);
                }

                if(sniff_frame.protocol_modulation & PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS)
                {
                    qb_data.append("RDR ");
                }else
                {
                    if(sniff_frame.protocol_modulation & PROTOCOL_MODULATION_TYPEA_MANCHESTER_106KBPS)
                    {
                        qb_data.append("TAG ");
                    }else if(sniff_frame.protocol_modulation & PROTOCOL_MODULATION_UNKNOWN)
                        qb_data.append("UKN ");
                }

                if(is_frame_contains_binary_data(&sniff_frame) == 1)
                {
                    if(sniff_frame.protocol_options & PROTOCOL_OPTIONS_PARITY)
                    {
                        /* Print Data as Hex Text */
                        t_ascii_hex val_ascii;
                        for (int i = 0; i < data_size; i++)
                        {
                            if(i > 0 && ((i+1)%2 == 0)) /* Display Parity Byte like Proxmark3 */
                            {
                                if(sniff_frame.data[i] == 0)
                                    qb_data.append("  ");
                                else
                                    qb_data.append("! ");
                            }else
                            {
                                val_ascii = BinHextoAscii(sniff_frame.data[i]);
                                qb_data.append(val_ascii.msb);
                                qb_data.append(val_ascii.lsb);
                            }
                        }
                    }else
                    {
                        /* Print Data as Hex Text */
                        t_ascii_hex val_ascii;
                        for (int i = 0; i < data_size; i++)
                        {
                            val_ascii = BinHextoAscii(sniff_frame.data[i]);
                            qb_data.append(val_ascii.msb);
                            qb_data.append(val_ascii.lsb);
                            qb_data.append(' ');
                        }
                    }
                }else
                {
                    if(AsciiEnabled == true)
                    {
                        /* Print Data as ASCII */
                        bin_to_ascii_txt(sniff_frame.data, data_size, (t_u8*)temp_string);
                        qb_data.append(temp_string);
                        qb_data.append(' ');
                    }
                    if(HexEnabled == true)
                    {
                        /* Print Data as Hex Text */
                        t_ascii_hex val_ascii;
                        for (int i = 0; i < data_size; i++)
                        {
                            val_ascii = BinHextoAscii(sniff_frame.data[i]);
                            qb_data.append(val_ascii.msb);
                            qb_data.append(val_ascii.lsb);
                            qb_data.append(' ');
                        }
                    }
                }

                if(is_frame_contains_end_timestamp(&sniff_frame) == true)
                {
                    sniff_frame_state = SNIFF_FRAME_END_TIMESTAMP_DECODE;
                }else
                {
                    sniff_frame_state = SNIFF_FRAME_INIT_DECODE;

                    if(findLiveRefresh == true)
                    {
                        QString m_find = findText;
                        if(findText.length() > 0)
                        {
                            QString str(qb_data);
                            if(str.indexOf(QRegularExpression(m_find), 0) != -1)
                            {
                                this->console_find->appendPlainText(str);
                            }
                        }
                    }
                    console->putData(qb_data);
                }
            }else
            {
                nb_byte_available = 0;
            }
        }

        if(sniff_frame_state == SNIFF_FRAME_END_TIMESTAMP_DECODE)
        {
            if(nb_byte_available >= SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES)
            {
                t_u8 data[SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES];

                nb_byte_read = stream->read((char*)data, SNIFF_FRAME_TIMESTAMP_DECODE_NB_BYTES);
                if(nb_byte_read > 0)
                {
                    directdisk->putData((char*)data, nb_byte_read);
                }
                nb_byte_available = stream->bytesAvailable();
                sniff_frame_state = sniff_frame_timestamp_decode(data, &sniff_frame, false);

                if(sniff_frame.end_of_frame_time.timestamp > sniff_frame.start_of_frame_time.timestamp)
                {
                    snprintf(temp_string, TEMP_STRING_LEN, "(delta %u) %08X",
                                    (sniff_frame.end_of_frame_time.timestamp - sniff_frame.start_of_frame_time.timestamp),
                                    sniff_frame.end_of_frame_time.timestamp);
                }else
                {
                    snprintf(temp_string, TEMP_STRING_LEN, "(delta %u) %08X",
                                    (sniff_frame.start_of_frame_time.timestamp - sniff_frame.end_of_frame_time.timestamp),
                                    sniff_frame.end_of_frame_time.timestamp);
                }
                qb_data.append(temp_string);

                if(findLiveRefresh == true)
                {
                    QString m_find = findText;
                    if(findText.length() > 0)
                    {
                        QString str(qb_data);
                        if(str.indexOf(QRegularExpression(m_find), 0) != -1)
                        {
                            this->console_find->appendPlainText(str);
                        }
                    }
                }
                console->putData(qb_data);
                qb_data.clear();
            }else
            {
                nb_byte_available = 0;
            }
        }
    }
}

void hydratooldialog::readData()
{
    unsigned long nb_bytes;

    sem.acquire();
    if (serial->isOpen())
    {
        nb_bytes = serial->bytesAvailable();
        if(RawEnabled == true)
        {
            raw_data();
        }else
        {
            sniff_trace(serial, 0);
        }
        serialport_timer_sem.acquire();
        serialport_nb_bytes_rx += nb_bytes - serial->bytesAvailable();
        serialport_nb_packets_rx += 1;
        serialport_timer_sem.release();
    }
    sem.release();
}

void hydratooldialog::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        ui->statusBar->showMessage("QSerialPort::ResourceError " + serial->errorString());
        if (serial->isOpen())
        {
            closeSerialPort();
            openSerialPort();
        }
    }
}

void hydratooldialog::on_actionClear_triggered()
{
    sem.acquire();
    console->clear();
    sem.release();
}

void hydratooldialog::on_actionResync_triggered()
{
    sem.acquire();
    if (serial->isOpen())
        serial->clear();
    sniff_frame_state = SNIFF_FRAME_INIT_DECODE;
    sem.release();
}

void hydratooldialog::initActionsConnections()
{
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(on_actionConnectDisconnect()));
    connect(ui->actionConfigure, SIGNAL(triggered()), settingsCom, SLOT(show()));
}

void hydratooldialog::on_actionChanTerminal_triggered()
{
    dockwidget_terminal_ext->show();
    terminal_chan->raise();
    terminal_chan->show();
}

void hydratooldialog::on_actionRaw_toggled(bool arg1)
{
    RawEnabled = arg1;
}

void hydratooldialog::on_actionAscii_toggled(bool arg1)
{
    AsciiEnabled = arg1;
}

void hydratooldialog::on_actionHex_toggled(bool arg1)
{
    HexEnabled = arg1;
}

void hydratooldialog::on_actionAutoScroll_toggled(bool arg1)
{
    AutoScrollEnabled = arg1;
    console->setLocalVerticalScrollBarMaxEnabled(AutoScrollEnabled);
}

void hydratooldialog::on_actionPCdateTime_toggled(bool arg1)
{
    PCdateTimeEnabled = arg1;

    if(arg1 == true)
    {
        terminal_chan->setPCdateTimeCheckState(Qt::Checked);
    }else
    {
        terminal_chan->setPCdateTimeCheckState(Qt::Unchecked);
    }
}

void hydratooldialog::on_loadFile_clicked()
{
    QString fileName;
    QFile file;
    QByteArray buffer;
    QBuffer stream;
    qint64 total_nb_bytes;
    qint64 nb_bytes;
    qint64 nb_bytes_prev;
    int decode_no_change_cnt;
    int modulo;
    int curr_loop;

    decode_no_change_cnt = 0;
    nb_bytes = 0;
    nb_bytes_prev = 0;

    if(this->loadfile_path.length() == 0)
    {
        this->loadfile_path = "./";
    }

    fileName = QFileDialog::getOpenFileName(this, "Select sniff binary file...", this->loadfile_path, "sniff binary files (*.bin);;All files (*.*)");
    if(!fileName.length()) return;

    this->loadfile_path = QFileInfo(fileName).path(); // store path for next time

    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);

    if(QFileInfo(fileName).suffix().toLower() == "bin")
    {
        sem.acquire();
            sniff_frame_state = SNIFF_FRAME_INIT_DECODE;
            memset(&sniff_frame, 0, sizeof(sniff_frame));
        sem.release();
        closeSerialPort();
        console->clear();

        total_nb_bytes = file.bytesAvailable();
        QProgressDialog progress("Loading in progress...", "Cancel", 0, total_nb_bytes, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(1000);

        curr_loop = 0;
        while(1)
        {
            modulo = curr_loop % 50;
            if(modulo == 0)
                progress.setValue(nb_bytes);

            /* Decode & Insert Trace */
            //this->ui->console->setUpdatesEnabled(false);
            sniff_trace(&file, 100);
            //this->ui->console->setUpdatesEnabled(true);

            nb_bytes = total_nb_bytes - file.bytesAvailable();
            if(nb_bytes != nb_bytes_prev)
            {
                nb_bytes_prev = nb_bytes;
                decode_no_change_cnt = 0;
            }else
            {
                decode_no_change_cnt++;
            }

            if (progress.wasCanceled())
            {
                break;
            }

            if(decode_no_change_cnt > 10)
            {
                break;
            }

            curr_loop++;
        }
        progress.setValue(total_nb_bytes);
    }else
    {
        console->clear();
        /* Load all like a text file */
        this->ui->console->putData(file.readAll());
    }
    file.close();
}

void hydratooldialog::on_saveFile_clicked()
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

        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream stream(&file);
            stream << ui->console->toPlainText();
            stream.flush();
            file.close();
        }
        else
        {
            QMessageBox::critical(this, tr("Error"), tr("Error to save file"));
            return;
        }
    }
}

void hydratooldialog::on_saveDirectToDisk_clicked()
{
    dockwidget_hydratool_directdisk_ext->show();
    directdisk->raise();
    directdisk->show();
}

void hydratooldialog::on_findValue_returnPressed()
{
    QString m_find;
    int nb_lines_added;

    findText = ui->findValue->text();
    m_find = findText;

    if(m_find.isEmpty())
    {
        this->console_find->appendPlainText("Search \"" + m_find + "\" (0 hits)");
        return;
    }

    /* Search */
    nb_lines_added = 0;
    {
        int modulo;
        int nb_lines;
        int curr_loop;
        QString plainTextEditContents = ui->console->toPlainText();
        QStringList lines = plainTextEditContents.split("\n");

        nb_lines = lines.size();
        QProgressDialog progress("Searching in progress...", "Cancel", 0, nb_lines, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(2000);

        curr_loop = 0;
        for(int i = 0; i < nb_lines; i++)
        {
            modulo = i % 500;
            if(modulo == 0)
                progress.setValue(curr_loop);

            if(lines.at(i).indexOf(QRegularExpression(m_find), 0) != -1)
            {
                this->console_find->appendPlainText(lines.at(i));
                nb_lines_added++;
            }

            if(modulo == 0)
            {
                if (progress.wasCanceled())
                {
                    break;
                }
            }
            curr_loop++;
        }
        progress.setValue(nb_lines);
    }

    if(nb_lines_added == 0)
        this->console_find->appendPlainText("Search \"" + m_find + "\" (0 hits)");
}

void hydratooldialog::on_findPredefBox_currentIndexChanged(int index)
{
    ui->findValue->setText(ui->findPredefBox->itemData(index).toString());
    this->console_find->clear();

    if(ui->findValue->text().length() > 0)
    {
        on_findValue_returnPressed();
    }
}

void hydratooldialog::on_findLiveRefreshCheckBox_clicked(bool checked)
{
    findLiveRefresh = checked;
}
