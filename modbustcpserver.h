#ifndef MODBUSTCPSERVER_H
#define MODBUSTCPSERVER_H

#include <QMainWindow>
#include <QNetworkInterface>
#include <QTcpServer>

QT_BEGIN_NAMESPACE
namespace Ui {
class ModbusServer;
}
class QTcpSocket;
QT_END_NAMESPACE

/// Коды функций
#define MB_TCP_R_COIL 0x01 // Функция чтения дискретных выходов
#define MB_TCP_R_DINPUT 0x02 // Функция чтения дискретных входов
#define MB_TCP_R_HOLDING 0x03 // Функция чтения 32-битных выходов
#define MB_TCP_R_INPUT 0x04 // Функция чтения 16-битных входов
#define MB_TCP_W_SINGLE_COIL 0x05 // Функция записи одного дискретного выхода
#define MB_TCP_W_SINGLE_HOLDING 0x06 // Функция записи одного 32-битного выхода
#define MB_TCP_W_MULTIPLE_COIL 0x0F // Запись нескольких дискретных выходов
#define MB_TCP_W_MULTIPLE_HOLDING 0x10 // Запись нескольких 32-битных выходов

/// Коды ошибок
#define MB_ILLEGAL_FUNCTION_ERR 0x1
#define MB_ILLEGAL_DATA_ADDRESS_ERR 0x2
#define MB_ILLEGAL_DATA_VALUE_ERR 0x3
#define MB_FAILURE_ASSOC_ERR 0x4
#define MB_ACKNOWLEDGE_ERR 0x5
#define MB_BUSY_ERR 0x6
#define MB_NACK_ERR 0x7

/// Количество входов\выходов
#define MB_UI_DIN_NUM 5
#define MB_UI_DOUT_NUM 4
#define MB_UI_2_BYTEIN_NUM 4
#define MB_UI_4_BYTEOUT_NUM 2

#define UI_INPUT_MASK_16 ">HH HH"
#define UI_INPUT_MASK_32 ">HH HH HH HH"

class ModbusServer : public QMainWindow {
  Q_OBJECT

public:
  explicit ModbusServer(QWidget *parent = nullptr);
  ~ModbusServer();

private:
  Ui::ModbusServer *ui;

  QList<int> m_discretInputAddrList = {10031, 10033, 10036, 10039, 10046};
  QList<int> m_discretOutputAddrList = {20122, 20123, 20124, 20125};
  QList<int> m_twoByteInputAddrList = {30569, 30570, 30579, 30580};
  QList<int> m_fourByteOutputAddrList = {40058, 40059, 40110, 40111};

  QList<bool> dInputList = {false, false, false, false, false};
  QList<bool> dOutputList = {false, false, false, false};
  QList<uint16_t> twoBInputList = {0, 0, 0, 0};
  QList<uint32_t> fourBOutputList = {0, 0};

  typedef struct {
    uint16_t transId;
    uint16_t protocolId;
    uint16_t transLen;
  } mbTcpInitTrans_t;

  QTcpServer *m_pServer;
  QStringList m_sendingData;

  QDataStream m_requestStream;

  bool m_isHandwriting;

  uint16_t m_transID;
  uint16_t const m_protocolID;

  QList<QTcpSocket *> m_incommingSocketsList;

private:
  void initServer();
  void sendData(QList<uint8_t> data);
  QList<uint8_t> prepareAnswer(QByteArray request);

private slots:
  void on_cb_isHandwriting_stateChanged(int arg1);

  void incommingConnection();
  void slotReadyRead();
  void slotDisconnected();
};
#endif // MODBUSTCPSERVER_H
