#include <QMessageBox>
#include <QPushButton>
#include <QTcpSocket>
#include <QTimer>

#include "modbustcpserver.h"
#include "ui_modbustcpserver.h"

ModbusServer::ModbusServer(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusServer), m_isHandwriting(false),
      m_protocolID(0) {

  ui->setupUi(this);
  m_incommingSocketsList = QList<QTcpSocket *>();

  m_transID = 0;

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
  //  in.setVersion(QDataStream::Qt_5_12);

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
      if (CRC == incommingCRC) {
        answer = prepareAnswer(request);
      } else {
        qDebug() << "CRC error!";
      }
      sendData(answer);
      break;
    }
  }
}

QList<uint8_t> ModbusServer::prepareAnswer(QByteArray request) {
  QList<uint8_t> answer;
  answer.clear();
  uint unitId = request.at(6);
  uint func = request.at(7);

  if (unitId == uint(ui->sbUnitId->value())) {
    switch (func) {
    // Read discrete outputs
    case MB_TCP_R_COIL: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_discretOutputAddrList.contains(regAddress)) {

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }
    // Read discrete inputs
    case MB_TCP_R_DINPUT: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_discretInputAddrList.contains(regAddress)) {

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }
    // Read 32b outputs
    case MB_TCP_R_HOLDING: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_fourByteOutputAddrList.contains(regAddress)) {

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }
    // Read 16b inputs
    case MB_TCP_R_INPUT: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_twoByteInputAddrList.contains(regAddress)) {

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }

    // Write discrete output
    case MB_TCP_W_SINGLE_COIL: {
      uint16_t regAddress;
      char buff[sizeof(uint16_t)];
      buff[0] = request.at(9);
      buff[1] = request.at(8);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_discretOutputAddrList.contains(regAddress)) {
        answer.append(unitId);
        answer.append(func);

        uint16_t reg;
        char regBuff[sizeof(uint16_t)];
        regBuff[0] = request.at(11);
        regBuff[1] = request.at(10);
        memcpy(&reg, &regBuff, sizeof(uint16_t));

        QSpinBox *sb = findChild<QSpinBox *>(
            "sbDOut" +
            QString::number(m_discretOutputAddrList.indexOf(regAddress)));
        if (sb) {
          if (reg == 0xFF00) {
            sb->setValue(1);
          } else {
            sb->setValue(0);
          }
        } else {
          qDebug() << "Cant find spinbox";
        }

        answer.append(buff[1]);
        answer.append(buff[0]);
        answer.append(regBuff[1]);
        answer.append(regBuff[0]);
        return answer;

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }

    // Write 32b output
    case MB_TCP_W_SINGLE_HOLDING: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_fourByteOutputAddrList.contains(regAddress)) {
        answer.append(unitId);
        answer.append(func);

        //        uint32_t reg;
        //        QLineEdit *le = findChild<QLineEdit *>(
        //            "leQOut" +
        //            QString::number(m_fourByteOutputAddrList.indexOf(regAddress)));
        //        if (le) {
        //          reg = le->text().toInt(nullptr, 16);
        //        } else {
        //          qDebug() << "Cant find spinbox";
        //        }

        //        char regBuff[sizeof(uint32_t)];
        //        memcpy(&regBuff, &reg, sizeof(uint32_t));

        //        answer.append(regBuff[0]);
        //        answer.append(regBuff[1]);
        //        answer.append(regBuff[2]);
        //        answer.append(regBuff[3]);
        return answer;
      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }

    // Write discrete outputs
    case MB_TCP_W_MULTIPLE_COIL: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_discretOutputAddrList.contains(regAddress)) {

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }
    // Write 32b outputs
    case MB_TCP_W_MULTIPLE_HOLDING: {
      uint16_t regAddress;
      char buff[2];
      buff[0] = request.at(8);
      buff[1] = request.at(9);
      memcpy(&regAddress, &buff, sizeof(uint16_t));

      if (m_fourByteOutputAddrList.contains(regAddress)) {

      } else {
        qDebug() << "MB_NACK_ERR";
        answer.append(unitId);
        answer.append(func | 0x80);
        answer.append(MB_NACK_ERR);
        return answer;
      }
      break;
    }
    default: {
      qDebug() << "MB_ILLEGAL_DATA_VALUE_ERR";
      answer.append(unitId);
      answer.append(func | 0x80);
      answer.append(MB_ILLEGAL_DATA_VALUE_ERR);
      return answer;
    }
    }
  } else {
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
  // Increment trans ID
  m_transID++;

  // Take first client socket from List
  QTcpSocket *clientConnection = m_incommingSocketsList.first();
  QByteArray block;
  block.clear();

  // Create dataStream to send data into socket
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_12);

  out << uint16_t(m_transID) << uint16_t(m_protocolID) << qint16(0);

  foreach (uint8_t ch, message) {
    out << ch;
  }

  // set commandSize
  out.device()->seek(4);
  out << quint16(block.size() + 2 - sizeof(qint16) * 3);

  // calc CRC
  qint16 CRC = qChecksum(block, block.size());
  uint8_t buff[sizeof(qint16)];
  memcpy(&buff, &CRC, sizeof(qint16));

  for (uint64_t i = 0; i < sizeof(qint16); i++) {
    block.append(buff[i]);
  }

  // write data in socket
  clientConnection->write(block);
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
