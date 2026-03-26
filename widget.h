#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QModbusRtuSerialClient>
#include <QModbusDataUnit>
#include <QSerialPort>
#include <QTranslator>
#include <QMap>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onReadClicked();
    void onWriteClicked();
    void onReadCoilClicked();
    void onWriteCoilClicked();
    void onLanguageToggled(bool checked);
    void onClearLog();
    void onModbusStateChanged(QModbusDevice::State state);
    void onModbusErrorOccurred(QModbusDevice::Error error);
    void onReadReady();
    void refreshPortList();

private:
    void retranslateUiText();
    void updateStatusBar(const QString &msg, bool ok = true);
    void appendLog(const QString &msg);
    bool validateInputs();
    void checkSerialPortAvailability();

    Ui::Widget *ui;
    QModbusRtuSerialClient *m_modbusClient = nullptr;
    QTranslator m_translator;
    bool m_isChinese = true;
    QTimer *m_portRefreshTimer = nullptr;

    // pending reply tracking
    QModbusReply *m_pendingReply = nullptr;
};

#endif // WIDGET_H
