#include "modbustcpserver.h"
#include "ui_modbustcpserver.h"

ModbusServer::ModbusServer(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusServer), m_isHandwriting(false) {

  ui->setupUi(this);
  m_incommingSocketsList = QList<QTcpSocket *>();

  memset(&m_dInputs, 0, sizeof(discretInputs_t));
  memset(&m_dOutputs, 0, sizeof(discretOutputs_t));
  memset(&m_doInputs, 0, sizeof(TwoBytesInputs_t));
  memset(&m_qOutputs, 0, sizeof(FourBytesOutputs_t));

  setWindowTitle("Сервер modbus-TCP");

  initServer();

  ui->textEdit->setReadOnly(true);

  for (int i = 0; i < MB_UI_DIN_NUM; i++) {
    QSpinBox *sb = findChild<QSpinBox *>("spinbDIn" + QString::number(i));
    if (sb) {
      sb->setReadOnly(true);
      sb->setMinimum(0);
      sb->setMaximum(1);
    }
  }

  for (int i = 0; i < MB_UI_DOUT_NUM; i++) {
    QSpinBox *sb = findChild<QSpinBox *>("sbDOut" + QString::number(i));
    if (sb) {
      sb->setReadOnly(true);
      sb->setMinimum(0);
      sb->setMaximum(1);
    }
  }

  for (int i = 0; i < MB_UI_2_BYTEIN_NUM; i++) {
    QSpinBox *sb = findChild<QSpinBox *>("sbDIn" + QString::number(i));
    if (sb) {
      sb->setReadOnly(true);
      sb->setMinimum(0);
      sb->setMaximum(UINT16_MAX);
    }
  }

  for (int i = 0; i < MB_UI_4_BYTEOUT_NUM; i++) {
    QLineEdit *le = findChild<QLineEdit *>("leQOut" + QString::number(i));
    if (le) {
      le->setReadOnly(true);
    }
  }
}

ModbusServer::~ModbusServer() { delete ui; }

/////////
/// \brief Server initial settings
///
void ModbusServer::initServer() {
  m_pServer = new QTcpServer(this);

  connect(m_pServer, &QTcpServer::newConnection, this,
          &ModbusServer::incommingConnection);

  if (!m_pServer->listen(QHostAddress::Any, 502)) {
    QMessageBox::critical(
        this, tr("Modbus Server"),
        tr("Unable to start the server: %1.").arg(m_pServer->errorString()));
    close();
    return;
  }
  QString ipAddress;
  QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
  // use the first non-localhost IPv4 address
  for (int i = 0; i < ipAddressesList.size(); ++i) {
    if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
        ipAddressesList.at(i).toIPv4Address()) {
      ipAddress = ipAddressesList.at(i).toString();
      break;
    }
  }
  // if we did not find one, use IPv4 localhost
  if (ipAddress.isEmpty())
    ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
  ui->lb_status->setText(tr("Сервер запущен\nIP: %1\nport: %2")
                             .arg(ipAddress)
                             .arg(m_pServer->serverPort()));
}

////////////////////////
/// \brief SLOT Create a new client socket, connect it with slots and put it in
/// List
/// \param description
///
void ModbusServer::incommingConnection() {
  QTcpSocket *socket = m_pServer->nextPendingConnection();
  ui->textEdit->append(QString("New connection with description: %1")
                           .arg(socket->socketDescriptor()));
  connect(socket, &QTcpSocket::readyRead, this, &ModbusServer::slotReadyRead);
  connect(socket, &QTcpSocket::disconnected, this,
          &ModbusServer::slotDisconnected);

  m_incommingSocketsList.append(socket);
}

////////////
/// \brief SLOT Delete client socket and remove its link from List when it was
/// disconnected
///
void ModbusServer::slotDisconnected() {
  QTcpSocket *socket = (QTcpSocket *)sender();

  m_incommingSocketsList.removeOne(socket);
  socket->deleteLater();
}

//////////
/// \brief ModbusServer::slotReadyRead
///
void ModbusServer::slotReadyRead() {
  mbTcpInitTrans_t initData;
  initData.transLen = 0;
  initData.protocolId = 0;
  initData.transId = 0;

  QTcpSocket *socket = (QTcpSocket *)sender();
  QDataStream in(socket);
  in.setVersion(QDataStream::Qt_5_12);

  if (in.status() == QDataStream::Ok) {
    ui->textEdit->append(QString("Read incomming request from %1...")
                             .arg(socket->socketDescriptor()));

    for (;;) {
      if (initData.transLen == 0) {
        if (socket->bytesAvailable() < 6) {
          break;
        }
        in >> initData.transId;
        ui->textEdit->append(
            QString("Incomming transaction with ID: %1").arg(initData.transId));
        in >> initData.protocolId;
        ui->textEdit->append(
            QString("Protocol ID: %1").arg(initData.protocolId));
        in >> initData.transLen;
        ui->textEdit->append(
            QString("Size of incomming request: %1").arg(initData.transLen));
      }
      if (socket->bytesAvailable() < initData.transLen) {
        ui->textEdit->append(QString("%1 bytes out of %2 are avaliable...")
                                 .arg(QString::number(socket->bytesAvailable()),
                                      QString::number(initData.transLen)));
        break;
      }
      ui->textEdit->append(QString("%1 bytes out of %2 are avaliable")
                               .arg(QString::number(socket->bytesAvailable()),
                                    QString::number(initData.transLen)));

      QByteArray request;
      request.clear();

      // Read CRC :_(
      char initBuff[sizeof(mbTcpInitTrans_t)];
      char tranIdBuff[2];
      memcpy(&tranIdBuff, &initData.transId, 2);
      char protocolIdBuff[2];
      memcpy(&protocolIdBuff, &initData.protocolId, 2);
      char tranLenBuff[2];
      memcpy(&tranLenBuff, &initData.transLen, 2);
      initBuff[0] = tranIdBuff[1];
      initBuff[1] = tranIdBuff[0];
      initBuff[2] = protocolIdBuff[1];
      initBuff[3] = protocolIdBuff[0];
      initBuff[4] = tranLenBuff[1];
      initBuff[5] = tranLenBuff[0];
      for (uint64_t i = 0; i < sizeof(mbTcpInitTrans_t); i++) {
        request.append(initBuff[i]);
      }

      for (int i = 0; i < initData.transLen; i++) {
        char byte;
        socket->read(&byte, sizeof(char));
        request.append(uint8_t(byte));
      }

      qint16 CRC = qChecksum(request, request.size() - 2);

      qint16 incommingCRC = 0;
      uint8_t crcBuff[sizeof(qint16)];

      crcBuff[0] = request.at(request.size() - 2);
      crcBuff[1] = request.at(request.size() - 1);

      memcpy(&incommingCRC, &crcBuff, sizeof(qint16));

      QList<uint8_t> answer;
      answer.clear();
      if(CRC == incommingCRC)
      {
          answer = prepareAnswer(request);
      } else
      {
          qDebug() << "CRC error!";
      }
      sendData(answer);
      break;
    }
  }
}

QList<uint8_t> ModbusServer::prepareAnswer(QByteArray request)
{
    QList<uint8_t> answer;
    answer.clear();
    uint unitId = request.at(6);
    uint func = request.at(7);

    if(unitId == uint(ui->sbUnitId->value()))
    {
        switch (func)
        {
        case MB_TCP_R_COIL:
        {

            break;
        }
        case MB_TCP_R_DINPUT:
        {

            break;
        }
        case MB_TCP_R_HOLDING:
        {

            break;
        }
        case MB_TCP_R_INPUT:
        {

            break;
        }
        case MB_TCP_W_SINGLE_COIL:
        {

            break;
        }
        case MB_TCP_W_SINGLE_HOLDING:
        {

            break;
        }
        case MB_TCP_W_MULTIPLE_COIL:
        {

            break;
        }
        case MB_TCP_W_MULTIPLE_HOLDING:
        {

            break;
        }
        default:
        {
            qDebug() << "MB_ILLEGAL_DATA_VALUE_ERR";
            answer.append(unitId);
            answer.append(func | 0x8);
            answer.append(MB_ILLEGAL_DATA_VALUE_ERR);
            return answer;
        }
        }
    }
    else
    {
        qDebug() << "MB_ILLEGAL_DATA_ADDRESS_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_ILLEGAL_DATA_ADDRESS_ERR);
        return answer;
    }

    return answer;
}

/////////////
/// \brief SLOT send response to client
///
void ModbusServer::sendData(QList<uint8_t> message) {
  // Take first client socket from List
  QTcpSocket *clientConnection = m_incommingSocketsList.first();
  QByteArray block;
  block.clear();

  //  // Put response data in container
  //  m_sendingData.append(ui->leSendData->text());

  // Create dataStream to send data into socket
  QDataStream out(&block, QIODevice::WriteOnly);
//  out.setVersion(QDataStream::Qt_5_12);

  // write data in byteArray through the dataStream
//  out << qint16(0) << message;
//  out.device()->seek(0);
//  out << quint16(block.size() - sizeof(qint16));

  foreach(uint8_t ch, message)
  {
      out << ch;
  }

  qint16 CRC = qChecksum(block, block.size());
  uint8_t buff[sizeof(qint16)];
  memcpy(&buff, &CRC, sizeof(qint16));

  for (uint64_t i = 0; i < sizeof(qint16); i++) {
      block.append(buff[i]);
  }

  // write data in socket
  qDebug() << block;
  clientConnection->write(block);
  //  clientConnection->disconnectFromHost();
}

///////////
/// \brief SLOT UI Disable automatic data changing in server UI and allow user
/// change inputs data
/// \param arg1
///
void ModbusServer::on_cb_isHandwriting_stateChanged(int arg1) {
  m_isHandwriting = bool(arg1);

  for (int i = 0; i < MB_UI_DIN_NUM; i++) {
    QSpinBox *sb = findChild<QSpinBox *>("spinbDIn" + QString::number(i));
    if (sb) {
      sb->setReadOnly(!m_isHandwriting);
    }
  }

  for (int i = 0; i < MB_UI_2_BYTEIN_NUM; i++) {
    QSpinBox *sb = findChild<QSpinBox *>("sbDIn" + QString::number(i));
    if (sb) {
      sb->setReadOnly(!m_isHandwriting);
    }
  }
}
