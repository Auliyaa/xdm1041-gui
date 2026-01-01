#include <xdm1041.h>

#include <QApplication>
#include <QSerialPortInfo>
#include <chrono>
#include <cmath>
#include <limits>

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
  close();
}

bool xdm1041_t::open(const QString& port)
{
  if (isOpen()) close();

  _cached_cont_thre = std::numeric_limits<double>::quiet_NaN();
  _rx_buffer.clear();
  _lastError.clear();

  _ser.setPortName(port);
  _ser.setBaudRate(QSerialPort::Baud115200);
  _ser.setDataBits(QSerialPort::Data8);
  _ser.setParity(QSerialPort::NoParity);
  _ser.setStopBits(QSerialPort::OneStop);
  _ser.setFlowControl(QSerialPort::NoFlowControl);
  return _ser.open(QIODevice::ReadWrite);
}

bool xdm1041_t::isOpen() const
{
  return _ser.isOpen();
}

void xdm1041_t::close()
{
  if (!_ser.isOpen()) return;

  _ser.close();
  _rx_buffer.clear();
}

QString xdm1041_t::lastError() const
{
  return _lastError;
}

void xdm1041_t::clearLastError()
{
  _lastError.clear();
}

void xdm1041_t::drainInput()
{
  if (!_ser.isOpen())
    return;

  while (_ser.bytesAvailable() > 0)
    _ser.readAll();

  _rx_buffer.clear();
}

bool xdm1041_t::write(const QByteArray& d, int)
{
  if (!_ser.isOpen())
  {
    _lastError = "serial port not open";
    return false;
  }

  drainInput();

  const auto written = _ser.write(d);
  if (written != d.size())
  {
    _lastError = "short write to serial port";
    return false;
  }

  if (!_ser.waitForBytesWritten(100))
  {
    _lastError = "write timeout";
    return false;
  }

  return true;
}

QString xdm1041_t::read(int timeo)
{
  using clock = std::chrono::steady_clock;
  const auto deadline = clock::now() + std::chrono::milliseconds(timeo);

  while (clock::now() < deadline)
  {
    if (_ser.waitForReadyRead(20))
    {
      _rx_buffer += _ser.readAll();

      const auto eol = _rx_buffer.indexOf('\n');
      if (eol >= 0)
      {
        const auto line = _rx_buffer.left(eol);
        _rx_buffer.remove(0, eol + 1);

        const auto res = QString::fromLatin1(line).trimmed();
        return res;
      }
    }

    QApplication::processEvents();
  }

  _lastError = "xdm1041_t::read() timeout";
  return {};
}

QString xdm1041_t::idn()
{
  static const QByteArray cmd{"*IDN?\r\n"};
  if (!write(cmd))
    return {};

  return read();
}

QString xdm1041_t::func()
{
  static const QByteArray cmd{"FUNC?\r\n"};
  if (!write(cmd))
    return {};

  const auto r = read().remove('"');
  if (r.isEmpty())
    _lastError = "invalid FUNC? reply";

  return r;
}

QString xdm1041_t::meas()
{
  static const QByteArray cmd{"MEAS?\r\n"};
  if (!write(cmd))
    return {};

  return read().remove('"');
}

double xdm1041_t::meas_num()
{
  static const QByteArray cmd{"MEAS?\r\n"};
  if (!write(cmd))
    return NAN;

  const auto r = read().remove('"');
  bool ok = false;
  const auto v = r.toDouble(&ok);

  if (!ok)
  {
    _lastError = "invalid MEAS? reply: " + r;
    return NAN;
  }

  return v;
}

xdm1041_t::speed_e xdm1041_t::speed()
{
  static const QByteArray cmd{"RATE?\r\n"};
  if (!write(cmd))
    return speed_slow;

  const auto r = read();
  if (r.startsWith("S")) return speed_slow;
  if (r.startsWith("M")) return speed_medium;
  if (r.startsWith("F")) return speed_fast;

  _lastError = "invalid RATE? reply";
  return speed_slow;
}

bool xdm1041_t::set_speed(speed_e s)
{
  switch (s)
  {
    case speed_slow:   return write("RATE S\r\n");
    case speed_medium: return write("RATE M\r\n");
    case speed_fast:   return write("RATE F\r\n");
  }
  return false;
}

double xdm1041_t::continuityThreshold()
{
  if (!std::isnan(_cached_cont_thre))
    return _cached_cont_thre;

  static const QByteArray cmd{"CONT:THRE?\r\n"};
  if (!write(cmd))
    return NAN;

  const auto r = read();
  bool ok = false;
  const auto v = r.toDouble(&ok);

  if (!ok)
  {
    _lastError = "invalid CONT:THRE? reply";
    return NAN;
  }

  _cached_cont_thre = v;
  return v;
}
