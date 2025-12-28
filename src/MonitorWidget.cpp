#include <MonitorWidget.h>

#include <QLineSeries>

#include "ui_MonitorWidget.h"

static constexpr const qulonglong MAX_VALUES = 50;

MonitorWidget::MonitorWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f),
    _ui(new Ui::MonitorWidget)
{
  _ui->setupUi(this);

  _timer.setInterval(300);
  connect(&_timer, &QTimer::timeout, this, &MonitorWidget::refreshTimeout);
  _timer.start();

  // init chart
  _series = new QLineSeries(_ui->chart->chart());
  _ui->chart->chart()->addSeries(_series);
  _ui->chart->chart()->createDefaultAxes();
  _ui->chart->chart()->legend()->hide();
  _ui->chart->setRenderHint(QPainter::Antialiasing, true);
  _ui->chart->chart()->setBackgroundBrush(QColor(0,0,0));

  _series->setBrush(QColor(215,195,213));
  QPen p;
  p.setColor(QColor(215,195,213));
  p.setWidth(2);
  _series->setPen(p);
  for (auto axe : _ui->chart->chart()->axes())
  {
    axe->setGridLineVisible(false);
    axe->setLinePen(QColor("white"));
    // axe->setLabelsVisible(false);
    axe->setLabelsColor(QColor("white"));

  }
  _ui->chart->chart()->axes(Qt::Horizontal).first()->setRange(0, MAX_VALUES);
  _ui->chart->chart()->axes(Qt::Horizontal).first()->setLabelsVisible(false);
}

MonitorWidget::~MonitorWidget()
{
}

void MonitorWidget::setRefreshInterval(int i)
{
  _timer.setInterval(i);
}

QString funcUnit(const QString& func)
{
  if (func == "RES") return "Ω";
  if (func == "CONT") return "";
  if (func == "DIOD") return "V";
  if (func == "CURR") return "A";
  if (func == "CURR AC") return "A";
  if (func == "VOLT") return "V";
  if (func == "VOLT AC") return "V";
  if (func == "CAP") return "F";
  if (func == "FREQ") return "Hz";

  return "";
}

QString funcName(const QString& func)
{
  if (func == "RES") return "Resistance";
  else if (func == "CONT") return "Continuity";
  else if (func == "DIOD") return "Diode";
  else if (func == "CURR") return "Current";
  else if (func == "CURR AC") return "Current (AC)";
  else if (func == "VOLT") return "Voltage";
  else if (func == "VOLT AC") return "Voltage (AC)";
  else if (func == "CAP") return "Capacitance";
  else if (func == "FREQ") return "Frequency";

  return func;
}

double transformUnit(const double v, QString& unit)
{
  // convert to p<unit> n<unit> µ<unit> m<unit> <unit> K<unit> M<unit>
  const QString baseUnit = unit;
  double res = ::abs(v);

  if (res < 0.001)
  { // milli
    unit = "m" + baseUnit;
    res *= 1000;
    if (res < 0.001)
    { // micro
      unit = "µ" + baseUnit;
      res *= 1000;
      if (res < 0.001)
      { // nano
        unit = "n" + baseUnit;
        res *= 1000;
        if (res < 0.001)
        { // pico
          unit = "p" + baseUnit;
          res *= 1000;
        }
      }
    }
  }
  else if (res > 1000)
  { // kilo
    unit = "K" + baseUnit;
    res /= 1000;
    if (res > 1000)
    { // mega
      unit = "M" + baseUnit;
      res /= 1000;
    }
  }

  return (v < 0 ? -1. : 1.) * res;
}

void MonitorWidget::refreshTimeout()
{
  if (!_xdm1041.isOpen())
  {
    // detect port
    const auto ports = xdm1041_t::listCOMPorts();
    if (ports.isEmpty())
    {
      qCritical() << "could not detect port for DMM";
      return;
    }
    for (const auto& port : ports)
    {
      qDebug() << "opening port " << port;
      if (!_xdm1041.open(port))
      {
        qCritical() << "failed to open port: " << port;
#ifdef __linux__
        qCritical() << "please make sure you have proper rights and are in the right group:";
        qCritical() << "$ stat " << port;
        qCritical() << "$ sudo gpasswd -a ${USER} <group>";
#endif
      }
      else break;
    }
  }

  // fetch values
  _xdm1041.clearLastError();
  const auto func = _xdm1041.func();
  const auto meas = _xdm1041.meas_num();
  const auto continuityThreshold = _xdm1041.continuityThreshold();
  if (!_xdm1041.lastError().isEmpty() || funcName(func).isEmpty())
  { // an error ocurred. stop here and let the code retry on next timeout
    // note: in some cases, we got no error but the returned function does not correspond to anything.
    //       consider this as an error and reset the DMM anyway
    qCritical() << _xdm1041.lastError();
    _xdm1041.close();
    return;
  }

  if (func != _oldFunc)
  { // func changed: clear buffers
    _values.clear();
    _oldFunc = func;
  }

  QString lbl = funcName(func) + ": ";
  if (func != "CONT")
  { // non-continuity mode: simply display the value at the right unit
    // show "overload" if value goes beyond allowed range
    if (meas >= 1e9)
    {
      lbl += "overload";
      _values.append(0.f); // avoid ruining the legend
    }
    else
    {
      auto unit = funcUnit(func);
      const auto v = transformUnit(meas, unit);
      lbl += QString::number(v,'f',2) + unit;
      _values.append(meas); // add the measured value as is to the chart
    }
  }
  else
  { // continuity mode: shows open or closed depending on configured threshold
    const auto continuityThreshold = _xdm1041.continuityThreshold();
    if (meas > continuityThreshold) lbl += "open";
    else lbl += "closed";

    // add either 0 (open) or 1 (closed) to the chart
    _values.append(meas > continuityThreshold ? 0 : 1);
  }

  // update label
  _ui->label->setText(lbl);

  // clear older chart values
  while (_values.size() > MAX_VALUES) _values.pop_front();

  // update serie & chart
  _series->clear();
  for (size_t ii=0; ii<_values.size(); ++ii)
  {
    _series->append(ii, _values[ii]);
  }
  const auto min = *std::min_element(_values.begin(), _values.end());
  const auto max = *std::max_element(_values.begin(), _values.end());
  _ui->chart->chart()->axes(Qt::Vertical).first()->setRange(min - abs(min)*0.1, max+max*0.1);
}