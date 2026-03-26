#include "widget.h"
#include "ui_widget.h"

#include <QSerialPortInfo>
#include <QModbusReply>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QTranslator>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    retranslateUiText();
    
    // Setup auto-refresh timer
    m_portRefreshTimer = new QTimer(this);
    connect(m_portRefreshTimer, &QTimer::timeout, this, [this]() {
        refreshPortList();
        checkSerialPortAvailability();
    });
    m_portRefreshTimer->start(1000); // Refresh every 1 second
    
    refreshPortList();

    // Modbus client
    m_modbusClient = new QModbusRtuSerialClient(this);
    connect(m_modbusClient, &QModbusClient::stateChanged,
            this, &Widget::onModbusStateChanged);
    connect(m_modbusClient, &QModbusClient::errorOccurred,
            this, &Widget::onModbusErrorOccurred);

    // Button signals
    connect(ui->btnConnect,    &QPushButton::clicked, this, &Widget::onConnectClicked);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &Widget::onDisconnectClicked);
    connect(ui->btnRead,       &QPushButton::clicked, this, &Widget::onReadClicked);
    connect(ui->btnWrite,      &QPushButton::clicked, this, &Widget::onWriteClicked);
    connect(ui->btnReadCoil,   &QPushButton::clicked, this, &Widget::onReadCoilClicked);
    connect(ui->btnWriteCoil,  &QPushButton::clicked, this, &Widget::onWriteCoilClicked);
    connect(ui->btnRefreshPort,&QPushButton::clicked, this, &Widget::refreshPortList);
    connect(ui->btnLang,       &QPushButton::toggled, this, &Widget::onLanguageToggled);
    connect(ui->btnClearLog,   &QPushButton::clicked, this, &Widget::onClearLog);

    onModbusStateChanged(QModbusDevice::UnconnectedState);
}

Widget::~Widget()
{
    if (m_modbusClient && m_modbusClient->state() != QModbusDevice::UnconnectedState)
        m_modbusClient->disconnectDevice();
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
// i18n
// ─────────────────────────────────────────────────────────────────────────────
void Widget::retranslateUiText()
{
    if (m_isChinese) {
        setWindowTitle("Modbus RTU 上位机");
        ui->grpSerial->setTitle("串口设置");
        ui->lblPort->setText("端口:");
        ui->lblBaud->setText("波特率:");
        ui->lblDataBits->setText("数据位:");
        ui->lblParity->setText("校验位:");
        ui->lblStopBits->setText("停止位:");
        ui->lblSlaveId->setText("从站地址:");
        ui->btnConnect->setText("连接");
        ui->btnDisconnect->setText("断开");
        ui->btnRefreshPort->setText("刷新");
        ui->lblStatus->setText("未连接");

        ui->grpHolding->setTitle("保持寄存器 (FC03/FC06)");
        ui->lblRegAddr->setText("起始地址:");
        ui->lblRegCount->setText("数量:");
        ui->lblWriteVal->setText("写入值:");
        ui->btnRead->setText("读取");
        ui->btnWrite->setText("写入");
        ui->leWriteVal->setPlaceholderText("例: 100 或 0x0064 或 10,20,30");

        ui->grpCoil->setTitle("线圈 / 离散输入 (FC01/FC05)");
        ui->lblCoilAddr->setText("起始地址:");
        ui->lblCoilCount->setText("数量:");
        ui->lblCoilWrite->setText("写入(0/1):");
        ui->btnReadCoil->setText("读取线圈");
        ui->btnWriteCoil->setText("写入线圈");

        ui->grpLog->setTitle("通信日志");
        ui->btnClearLog->setText("清空日志");
        ui->btnLang->setText("EN");

        // Table headers
        ui->tableHolding->setHorizontalHeaderLabels({"地址", "十进制", "十六进制"});
        ui->tableCoil->setHorizontalHeaderLabels({"地址", "值"});
    } else {
        setWindowTitle("Modbus RTU Master");
        ui->grpSerial->setTitle("Serial Settings");
        ui->lblPort->setText("Port:");
        ui->lblBaud->setText("Baud Rate:");
        ui->lblDataBits->setText("Data Bits:");
        ui->lblParity->setText("Parity:");
        ui->lblStopBits->setText("Stop Bits:");
        ui->lblSlaveId->setText("Slave ID:");
        ui->btnConnect->setText("Connect");
        ui->btnDisconnect->setText("Disconnect");
        ui->btnRefreshPort->setText("Refresh");
        ui->lblStatus->setText("Disconnected");

        ui->grpHolding->setTitle("Holding Registers (FC03/FC06)");
        ui->lblRegAddr->setText("Start Addr:");
        ui->lblRegCount->setText("Count:");
        ui->lblWriteVal->setText("Write Value:");
        ui->btnRead->setText("Read");
        ui->btnWrite->setText("Write");
        ui->leWriteVal->setPlaceholderText("e.g. 100, 0x0064, or 10,20,30");

        ui->grpCoil->setTitle("Coil / Discrete Input (FC01/FC05)");
        ui->lblCoilAddr->setText("Start Addr:");
        ui->lblCoilCount->setText("Count:");
        ui->lblCoilWrite->setText("Write(0/1):");
        ui->btnReadCoil->setText("Read Coil");
        ui->btnWriteCoil->setText("Write Coil");

        ui->grpLog->setTitle("Communication Log");
        ui->btnClearLog->setText("Clear Log");
        ui->btnLang->setText("中");

        ui->tableHolding->setHorizontalHeaderLabels({"Address", "Decimal", "Hex"});
        ui->tableCoil->setHorizontalHeaderLabels({"Address", "Value"});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────
void Widget::refreshPortList()
{
    ui->cmbPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &info : ports)
        ui->cmbPort->addItem(info.portName(), info.portName());
}

void Widget::checkSerialPortAvailability()
{
    if (m_modbusClient->state() != QModbusDevice::ConnectedState)
        return;

    // Get the current port name from the device
    QString portName = m_modbusClient->connectionParameter(QModbusDevice::SerialPortNameParameter).toString();
    
    // Check if current port still exists
    bool portExists = false;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &info : ports) {
        if (info.portName() == portName) {
            portExists = true;
            break;
        }
    }

    // Auto disconnect if port is gone
    if (!portExists) {
        m_modbusClient->disconnectDevice();
        appendLog(m_isChinese ? "设备已移除，连接已断开" : "Device removed, disconnected");
    }
}

void Widget::onLanguageToggled(bool /*checked*/)
{
    m_isChinese = !m_isChinese;
    retranslateUiText();
    
    // Update status bar with current state
    if (m_modbusClient->state() == QModbusDevice::ConnectedState) {
        ui->lblIndicator->setStyleSheet("background-color:#48BB78; border-radius:8px;");
        updateStatusBar(m_isChinese ? "已连接" : "Connected", true);
    } else if (m_modbusClient->state() == QModbusDevice::UnconnectedState) {
        ui->lblIndicator->setStyleSheet("background-color:#FC8181; border-radius:8px;");
        updateStatusBar(m_isChinese ? "未连接" : "Disconnected", false);
    } else {
        ui->lblIndicator->setStyleSheet("background-color:#F6AD55; border-radius:8px;");
        updateStatusBar(m_isChinese ? "连接中..." : "Connecting...", true);
    }
}

void Widget::onModbusStateChanged(QModbusDevice::State state)
{
    bool connected = (state == QModbusDevice::ConnectedState);
    ui->btnConnect->setEnabled(!connected);
    ui->btnDisconnect->setEnabled(connected);
    ui->btnRead->setEnabled(connected);
    ui->btnWrite->setEnabled(connected);
    ui->btnReadCoil->setEnabled(connected);
    ui->btnWriteCoil->setEnabled(connected);

    // Serial config widgets
    ui->cmbPort->setEnabled(!connected);
    ui->cmbBaud->setEnabled(!connected);
    ui->cmbDataBits->setEnabled(!connected);
    ui->cmbParity->setEnabled(!connected);
    ui->cmbStopBits->setEnabled(!connected);

    if (connected) {
        ui->lblIndicator->setStyleSheet("background-color:#48BB78; border-radius:8px;");
        updateStatusBar(m_isChinese ? "已连接" : "Connected", true);
        appendLog(m_isChinese ? "串口已连接" : "Connected");
    } else if (state == QModbusDevice::UnconnectedState) {
        ui->lblIndicator->setStyleSheet("background-color:#FC8181; border-radius:8px;");
        updateStatusBar(m_isChinese ? "未连接" : "Disconnected", false);
    } else {
        ui->lblIndicator->setStyleSheet("background-color:#F6AD55; border-radius:8px;");
        updateStatusBar(m_isChinese ? "连接中..." : "Connecting...", true);
    }
}

void Widget::onModbusErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError) return;
    QString msg = m_modbusClient->errorString();
    appendLog((m_isChinese ? QString("错误: ") : QString("Error: ")) + msg);
    updateStatusBar(msg, false);
}

void Widget::onConnectClicked()
{
    QString portName = ui->cmbPort->currentData().toString();
    if (portName.isEmpty()) {
        QMessageBox::warning(this,
            m_isChinese ? "警告" : "Warning",
            m_isChinese ? "请选择串口端口！" : "Please select a serial port!");
        return;
    }

    m_modbusClient->setConnectionParameter(
        QModbusDevice::SerialPortNameParameter, portName);

    m_modbusClient->setConnectionParameter(
        QModbusDevice::SerialBaudRateParameter,
        ui->cmbBaud->currentText().toInt());

    int dataBits = ui->cmbDataBits->currentText().toInt();
    m_modbusClient->setConnectionParameter(
        QModbusDevice::SerialDataBitsParameter, dataBits);

    QSerialPort::Parity parity = QSerialPort::NoParity;
    switch (ui->cmbParity->currentIndex()) {
        case 1: parity = QSerialPort::EvenParity; break;
        case 2: parity = QSerialPort::OddParity;  break;
        default: break;
    }
    m_modbusClient->setConnectionParameter(
        QModbusDevice::SerialParityParameter, parity);

    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    if (ui->cmbStopBits->currentText() == "2")
        stopBits = QSerialPort::TwoStop;
    m_modbusClient->setConnectionParameter(
        QModbusDevice::SerialStopBitsParameter, stopBits);

    m_modbusClient->setTimeout(500);
    m_modbusClient->setNumberOfRetries(2);

    if (!m_modbusClient->connectDevice()) {
        QString err = m_modbusClient->errorString();
        appendLog((m_isChinese ? QString("连接失败: ") : QString("Connect failed: ")) + err);
        updateStatusBar(err, false);
    }
}

void Widget::onDisconnectClicked()
{
    m_modbusClient->disconnectDevice();
    appendLog(m_isChinese ? "串口已断开" : "Disconnected");
}

void Widget::onReadClicked()
{
    int slaveId  = ui->spnSlaveId->value();
    int startAddr = ui->spnRegAddr->value();
    int count     = ui->spnRegCount->value();

    QModbusDataUnit request(QModbusDataUnit::HoldingRegisters, startAddr, count);
    if (auto *reply = m_modbusClient->sendReadRequest(request, slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError) {
                    const QModbusDataUnit unit = reply->result();
                    ui->tableHolding->setRowCount(unit.valueCount());
                    for (int i = 0; i < (int)unit.valueCount(); ++i) {
                        quint16 val = unit.value(i);
                        ui->tableHolding->setItem(i, 0, new QTableWidgetItem(
                            QString::number(unit.startAddress() + i)));
                        ui->tableHolding->setItem(i, 1, new QTableWidgetItem(
                            QString::number(val)));
                        ui->tableHolding->setItem(i, 2, new QTableWidgetItem(
                            QString("0x%1").arg(val, 4, 16, QChar('0')).toUpper()));
                    }
                    appendLog(m_isChinese
                        ? QString("读寄存器成功  地址%1  数量%2").arg(unit.startAddress()).arg(unit.valueCount())
                        : QString("Read registers OK  addr=%1  count=%2").arg(unit.startAddress()).arg(unit.valueCount()));
                } else if (reply->error() != QModbusDevice::TimeoutError) {
                    appendLog((m_isChinese ? QString("读寄存器失败: ") : QString("Read failed: ")) + reply->errorString());
                    updateStatusBar(reply->errorString(), false);
                } else {
                    appendLog(m_isChinese ? "读寄存器超时" : "Read timeout");
                    updateStatusBar(m_isChinese ? "读取超时" : "Timeout", false);
                }
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    } else {
        appendLog((m_isChinese ? QString("发送失败: ") : QString("Send failed: ")) + m_modbusClient->errorString());
    }
}

void Widget::onWriteClicked()
{
    int slaveId   = ui->spnSlaveId->value();
    int startAddr = ui->spnRegAddr->value();
    QString rawVal = ui->leWriteVal->text().trimmed();

    // Support comma-separated values for multiple registers
    QStringList parts = rawVal.split(',', Qt::SkipEmptyParts);
    QVector<quint16> values;
    bool ok = true;
    for (const QString &p : parts) {
        QString s = p.trimmed();
        quint16 v;
        if (s.startsWith("0x") || s.startsWith("0X"))
            v = s.toUShort(&ok, 16);
        else
            v = s.toUShort(&ok, 10);
        if (!ok) break;
        values.append(v);
    }

    if (!ok || values.isEmpty()) {
        QMessageBox::warning(this,
            m_isChinese ? "警告" : "Warning",
            m_isChinese ? "请输入有效的寄存器值（支持逗号分隔多个值，支持0x十六进制）"
                        : "Please enter valid register value(s). Comma-separated, 0x hex supported.");
        return;
    }

    QModbusDataUnit request(QModbusDataUnit::HoldingRegisters, startAddr, values.size());
    for (int i = 0; i < values.size(); ++i)
        request.setValue(i, values[i]);

    if (auto *reply = m_modbusClient->sendWriteRequest(request, slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply, startAddr]() {
                if (reply->error() == QModbusDevice::NoError) {
                    appendLog(m_isChinese
                        ? QString("写寄存器成功  地址%1").arg(startAddr)
                        : QString("Write registers OK  addr=%1").arg(startAddr));
                    updateStatusBar(m_isChinese ? "写入成功" : "Write OK", true);
                } else {
                    appendLog((m_isChinese ? QString("写寄存器失败: ") : QString("Write failed: ")) + reply->errorString());
                    updateStatusBar(reply->errorString(), false);
                }
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    } else {
        appendLog((m_isChinese ? QString("发送失败: ") : QString("Send failed: ")) + m_modbusClient->errorString());
    }
}

void Widget::onReadCoilClicked()
{
    int slaveId   = ui->spnSlaveId->value();
    int startAddr = ui->spnCoilAddr->value();
    int count     = ui->spnCoilCount->value();

    QModbusDataUnit request(QModbusDataUnit::Coils, startAddr, count);
    if (auto *reply = m_modbusClient->sendReadRequest(request, slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError) {
                    const QModbusDataUnit unit = reply->result();
                    ui->tableCoil->setRowCount(unit.valueCount());
                    for (int i = 0; i < (int)unit.valueCount(); ++i) {
                        ui->tableCoil->setItem(i, 0, new QTableWidgetItem(
                            QString::number(unit.startAddress() + i)));
                        ui->tableCoil->setItem(i, 1, new QTableWidgetItem(
                            unit.value(i) ? "1 (ON)" : "0 (OFF)"));
                    }
                    appendLog(m_isChinese
                        ? QString("读线圈成功  地址%1  数量%2").arg(unit.startAddress()).arg(unit.valueCount())
                        : QString("Read coils OK  addr=%1  count=%2").arg(unit.startAddress()).arg(unit.valueCount()));
                } else {
                    appendLog((m_isChinese ? QString("读线圈失败: ") : QString("Read coils failed: ")) + reply->errorString());
                    updateStatusBar(reply->errorString(), false);
                }
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    } else {
        appendLog((m_isChinese ? QString("发送失败: ") : QString("Send failed: ")) + m_modbusClient->errorString());
    }
}

void Widget::onWriteCoilClicked()
{
    int slaveId   = ui->spnSlaveId->value();
    int coilAddr  = ui->spnCoilAddr->value();
    int coilVal   = ui->spnCoilWrite->value();

    QModbusDataUnit request(QModbusDataUnit::Coils, coilAddr, 1);
    request.setValue(0, coilVal ? 1 : 0);

    if (auto *reply = m_modbusClient->sendWriteRequest(request, slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply, coilAddr]() {
                if (reply->error() == QModbusDevice::NoError) {
                    appendLog(m_isChinese
                        ? QString("写线圈成功  地址%1").arg(coilAddr)
                        : QString("Write coil OK  addr=%1").arg(coilAddr));
                    updateStatusBar(m_isChinese ? "线圈写入成功" : "Coil Write OK", true);
                } else {
                    appendLog((m_isChinese ? QString("写线圈失败: ") : QString("Write coil failed: ")) + reply->errorString());
                    updateStatusBar(reply->errorString(), false);
                }
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    } else {
        appendLog((m_isChinese ? QString("发送失败: ") : QString("Send failed: ")) + m_modbusClient->errorString());
    }
}

void Widget::onClearLog()
{
    ui->teLog->clear();
}

void Widget::onReadReady()
{
    // handled inside lambda in onReadClicked
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
void Widget::updateStatusBar(const QString &msg, bool ok)
{
    ui->lblStatus->setText(msg);
    ui->lblStatus->setStyleSheet(ok
        ? "background-color:#C6F6D5; color:#276749; border-radius:4px; padding:4px 10px;"
        : "background-color:#FED7D7; color:#9B2335; border-radius:4px; padding:4px 10px;");
}

void Widget::appendLog(const QString &msg)
{
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    ui->teLog->append(QString("[%1] %2").arg(ts, msg));
}

bool Widget::validateInputs()
{
    return true;
}
