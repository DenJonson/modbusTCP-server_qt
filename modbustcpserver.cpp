#include "modbustcpserver.h"
#include "ui_modbustcpserver.h"

ModbusServer::ModbusServer(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusServer), m_isHandwriting(false),
      m_clientMessageSize(0) {

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
  QTcpSocket *socket = (QTcpSocket *)sender();
  QDataStream in(socket);
  in.setVersion(QDataStream::Qt_5_12);

  if (in.status() == QDataStream::Ok) {
    ui->textEdit->append(QString("Read incomming request from %1...")
                             .arg(socket->socketDescriptor()));

    for (;;) {
      if (m_clientMessageSize == 0) {
        if (socket->bytesAvailable() < 2) {
          break;
        }
        in >> m_clientMessageSize;
        ui->textEdit->append(
            QString("Size of incomming request: %1").arg(m_clientMessageSize));
      }
      if (socket->bytesAvailable() < m_clientMessageSize) {
        ui->textEdit->append(QString("%1 bytes out of %2 are avaliable...")
                                 .arg(QString::number(socket->bytesAvailable()),
                                      QString::number(m_clientMessageSize)));
        break;
      }
      ui->textEdit->append(QString("%1 bytes out of %2 are avaliable")
                               .arg(QString::number(socket->bytesAvailable()),
                                    QString::number(m_clientMessageSize)));

      QByteArray request;
      request.clear();
      for (int i = 0; i < m_clientMessageSize; i++) {
        char byte;
        socket->read(&byte, sizeof(char));
        request.append(uint8_t(byte));
      }

      ui->textEdit->append("Request: " + QString::number(request.toUInt()));
      ui->leLastRequest->setText(QString::number(request.toUInt()));
      m_clientMessageSize = 0;
      sendData();
      break;
    }
  }
}

/////////////
/// \brief SLOT send response to client
///
void ModbusServer::sendData() {
  // Take first client socket from List
  QTcpSocket *clientConnection = m_incommingSocketsList.first();
  QByteArray block;
  block.clear();

  //  // Put response data in container
  //  m_sendingData.append(ui->leSendData->text());

  // Create dataStream to send data into socket
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_12);

  // write data in byteArray through the dataStream
  out << qint16(0) << ui->leSendData->text();
  out.device()->seek(0);
  out << quint16(block.size() - sizeof(qint16));

  // write data in socket
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
