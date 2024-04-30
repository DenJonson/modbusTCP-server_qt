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

/// адреса доступа к данным
#define MB_TCP_DINPUT_ADDR_1 10031
#define MB_TCP_DINPUT_ADDR_2 10033
#define MB_TCP_DINPUT_ADDR_3 10036
#define MB_TCP_DINPUT_ADDR_4 10039
#define MB_TCP_DINPUT_ADDR_5 10046

#define MB_TCP_DOUTPUT_ADDR_1 20122
#define MB_TCP_DOUTPUT_ADDR_2 20123
#define MB_TCP_DOUTPUT_ADDR_3 20124
#define MB_TCP_DOUTPUT_ADDR_4 20125

#define MB_TCP_TWOBYTEIN_ADDR_1 30569
#define MB_TCP_TWOBYTEIN_ADDR_2 30570
#define MB_TCP_TWOBYTEIN_ADDR_3 30579
#define MB_TCP_TWOBYTEIN_ADDR_4 30580

#define MB_TCP_FOURBYTEOUT_ADDR_1_1 40058
#define MB_TCP_FOURBYTEOUT_ADDR_1_2 40059
#define MB_TCP_FOURBYTEOUT_ADDR_2_1 40110
#define MB_TCP_FOURBYTEOUT_ADDR_2_2 40111

class ModbusServer : public QMainWindow {
  Q_OBJECT

public:
  explicit ModbusServer(QWidget *parent = nullptr);
  ~ModbusServer();

private:
  Ui::ModbusServer *ui;

  typedef struct {
    uint16_t transId;
    uint16_t protocolId;
    uint16_t transLen;
  } mbTcpInitTrans_t;

  typedef struct {
    bool dInput1;
    bool dInput2;
    bool dInput3;
    bool dInput4;
    bool dInput5;
  } discretInputs_t;

  typedef struct {
    bool dOutput1;
    bool dOutput2;
    bool dOutput3;
    bool dOutput4;
  } discretOutputs_t;

  typedef struct {
    uint16_t dDInput1;
    uint16_t dDInput2;
    uint16_t dDInput3;
    uint16_t dDInput4;
  } TwoBytesInputs_t;

  typedef struct {
    uint32_t dQOutput1;
    uint32_t dQOutput2;
  } FourBytesOutputs_t;

  QTcpServer *m_pServer;
  QStringList m_sendingData;

  discretInputs_t m_dInputs;
  discretOutputs_t m_dOutputs;
  TwoBytesInputs_t m_doInputs;
  FourBytesOutputs_t m_qOutputs;

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
