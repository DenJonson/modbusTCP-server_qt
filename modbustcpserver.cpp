#include "modbustcpserver.h"
#include "ui_modbustcpserver.h"

ModbusServer::ModbusServer(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ModbusServer), m_isHandwriting(false) {
  ui->setupUi(this);

  memset(&m_dInputs, 0, sizeof(discretInputs_t));
  memset(&m_dOutputs, 0, sizeof(discretOutputs_t));
  memset(&m_doInputs, 0, sizeof(TwoBytesInputs_t));
  memset(&m_qOutputs, 0, sizeof(FourBytesOutputs_t));

  setWindowTitle("Сервер modbus-TCP");

  initServer();

  for (int i = 0; i < MB_UI_DIN_NUM; i++) {
    QCheckBox *cb = findChild<QCheckBox *>("cbDIn" + QString::number(i));
    if (cb) {
      cb->setEnabled(false);
    }
  }

  for (int i = 0; i < MB_UI_DOUT_NUM; i++) {
    QCheckBox *cb = findChild<QCheckBox *>("cbDOut" + QString::number(i));
    if (cb) {
      cb->setEnabled(false);
    }
  }

  for (int i = 0; i < MB_UI_2_BYTEIN_NUM; i++) {
    QSpinBox *sb = findChild<QSpinBox *>("sbDIn" + QString::number(i));
    if (sb) {
      sb->setEnabled(false);
      sb->setMinimum(0);
      sb->setMaximum(UINT16_MAX);
    }
  }

  for (int i = 0; i < MB_UI_4_BYTEOUT_NUM; i++) {
    QLineEdit *le = findChild<QLineEdit *>("leQOut" + QString::number(i));
    if (le) {
      le->setEnabled(false);
    }
  }
}

ModbusServer::~ModbusServer() { delete ui; }

/////////////
/// \brief Слот, обрабатывающий запрос от клиента
///
void ModbusServer::sendData() {
  QTcpSocket *clientConnection = m_pServer->nextPendingConnection();
  connect(clientConnection, &QAbstractSocket::disconnected, clientConnection,
          &QObject::deleteLater);

  QDataStream requestStream(clientConnection);
  requestStream.startTransaction();

  //  QByteArray request;
  //  //  requestStream >> request;
  //  request = clientConnection->readAll();

  //  qDebug() << request;

  int request = -1;
  requestStream >> request;

  ui->lineEdit->setText(QString::number(request));

  m_requestStream.setDevice(nullptr);

  QByteArray block;

  m_sendingData.append(ui->leSendData->text());

  QDataStream out(&block, QIODevice::WriteOnly);
  //  out.setVersion(QDataStream::Qt_5_10);

  out << m_sendingData.last();

  clientConnection->write(block);
  clientConnection->disconnectFromHost();
  clientConnection = nullptr;
}

/////////
/// \brief ModbusServer::initServer
///
void ModbusServer::initServer() {
  m_pServer = new QTcpServer(this);

  connect(m_pServer, &QTcpServer::newConnection, this, &ModbusServer::sendData);

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

void ModbusServer::on_cb_isHandwriting_stateChanged(int arg1) {
  if (arg1) {
    m_isHandwriting = true;

    for (int i = 0; i < MB_UI_DIN_NUM; i++) {
      QCheckBox *cb = findChild<QCheckBox *>("cbDIn" + QString::number(i));
      if (cb) {
        cb->setEnabled(m_isHandwriting);
      }
    }

    for (int i = 0; i < MB_UI_2_BYTEIN_NUM; i++) {
      QSpinBox *sb = findChild<QSpinBox *>("sbDIn" + QString::number(i));
      if (sb) {
        sb->setEnabled(m_isHandwriting);
      }
    }
  } else {
    m_isHandwriting = false;

    for (int i = 0; i < MB_UI_DIN_NUM; i++) {
      QCheckBox *cb = findChild<QCheckBox *>("cbDIn" + QString::number(i));
      if (cb) {
        cb->setEnabled(m_isHandwriting);
      }
    }

    for (int i = 0; i < MB_UI_2_BYTEIN_NUM; i++) {
      QSpinBox *sb = findChild<QSpinBox *>("sbDIn" + QString::number(i));
      if (sb) {
        sb->setEnabled(m_isHandwriting);
      }
    }
  }
}
