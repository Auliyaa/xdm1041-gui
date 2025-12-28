#include <xdm1041.h>

#include <QApplication>
#include <QSerialPortInfo>
#include <QDebug>

QStringList xdm1041_t::listCOMPorts()
{
  QStringList res;
  for (const auto& info : QSerialPortInfo::availablePorts())
  {
    if (!info.hasVendorIdentifier() || !info.hasProductIdentifier())
    {
      continue;
    }
    if (info.vendorIdentifier() == 0x1A86 && 
        info.productIdentifier() == 0x7523)
    {
#ifdef __linux__
  res.append(info.systemLocation());
#else
      res.append(info.portName());
#endif
    }
  }

  return res;
}

xdm1041_t::xdm1041_t(QObject* parent)
  : QObject(parent)
{
}

xdm1041_t::~xdm1041_t()
{
}

bool xdm1041_t::open(const QString& port)
{
  if (isOpen()) close();
  _ser.setPortName(port);
  _ser.setBaudRate(QSerialPort::Baud115200);
  _ser.setDataBits(QSerialPort::Data8);
  _ser.setParity(QSerialPort::NoParity);
  _ser.setStopBits(QSerialPort::OneStop);
  _ser.setFlowControl(QSerialPort::NoFlowControl);
  return _ser.open(QIODevice::QIODevice::ReadWrite);
}

bool xdm1041_t::isOpen() const
{
  return _ser.isOpen();
}

void xdm1041_t::close()
{
  if (!isOpen()) return;
  _ser.close();
}

QString xdm1041_t::idn()
{
  static const QByteArray cmd{"*IDN?\r\n"};
  if (write(cmd)) return read();
  return "";
}

QString xdm1041_t::func()
{
  static const QByteArray cmd{"FUNC?\r\n"};
  if (write(cmd)) return read().remove("\"");
  return "";
}

QString xdm1041_t::meas()
{
  static const QByteArray cmd{"MEAS?\r\n"};
  if (write(cmd)) return read().remove("\"");
  return "";
}

double xdm1041_t::meas_num()
{
  static const QByteArray cmd{"MEAS?\r\n"};
  if (write(cmd)) return read().remove("\"").toDouble();
  return NAN;
}

xdm1041_t::speed_e xdm1041_t::speed()
{
  static const QByteArray cmd{"RATE?\r\n"};
  if (write(cmd))
  {
    const auto r = read();
    if (r.startsWith("S")) return speed_e::speed_slow;
    if (r.startsWith("M")) return speed_e::speed_medium;
    if (r.startsWith("F")) return speed_e::speed_fast;
  }
  return speed_e::speed_slow;
}

bool xdm1041_t::set_speed(speed_e s)
{
  switch(s)
  {
    case decltype(s)::speed_slow: return write("RATE S\r\n");
    case decltype(s)::speed_medium: return write("RATE M\r\n");
    case decltype(s)::speed_fast: return write("RATE F\r\n");
  }
  return true;
}

double xdm1041_t::continuityThreshold()
{
  static const QByteArray cmd{"CONT:THRE?\r\n"};
  if (write(cmd)) return read().toDouble();
  return NAN;
}

QString xdm1041_t::lastError() const
{
  return _lastError;
}

void xdm1041_t::clearLastError()
{
  _lastError = "";
}

bool xdm1041_t::write(const QByteArray& d, int timeo)
{
  using clock = std::chrono::steady_clock;

  const auto tgt_timeo = clock::now() + std::chrono::milliseconds(timeo);
  _ser.write(d);
  while (clock::now()<tgt_timeo)
  {
    if (_ser.waitForBytesWritten(10)) return true;
    QApplication::processEvents(); // avoid hanging the whole event loop
  }
  _lastError = "xdm1041_t::write() timeout";
  return false;
}

QString xdm1041_t::read(int timeo)
{
  using clock = std::chrono::steady_clock;

  const auto tgt_timeo = clock::now() + std::chrono::milliseconds(timeo);
  while (clock::now()<tgt_timeo)
  {
    if (_ser.waitForReadyRead(10) && _ser.canReadLine())
    {
      return QString::fromLatin1(_ser.readLine()).trimmed();
    }
    QApplication::processEvents(); // avoid hanging the whole event loop
  }
  _lastError = "xdm1041_t::read() timeout";
  return QByteArray{};
}