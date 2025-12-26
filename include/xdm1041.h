#pragma once

#include <QObject>
#include <QStringList>
#include <QSerialPort>

class xdm1041_t: public QObject
{
  Q_OBJECT
public:
  static QStringList listCOMPorts();

  xdm1041_t(QObject* parent=nullptr);
  virtual ~xdm1041_t();

  bool open(const QString& port);
  bool isOpen() const;
  void close();

  enum speed_e
  {
    speed_slow,
    speed_medium,
    speed_fast
  };

  QString idn();
  QString func();
  QString meas();
  double meas_num();
  speed_e speed();
  bool set_speed(speed_e);
  double continuityThreshold();

  QString lastError() const;
  void clearLastError();

private:
  QSerialPort _ser;
  QString _lastError;

  bool write(const QByteArray&, int timeo=1000);
  QString read(int timeo=1000);
};