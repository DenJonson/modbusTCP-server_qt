#include "modbustcpserver.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  ModbusServer w;
  w.show();
  return a.exec();
}
