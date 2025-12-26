#pragma once

#include <QFrame>
#include <QTimer>
#include <QChart>

#include <xdm1041.h>

namespace Ui
{
  class MonitorWidget;
}

class QLineSeries;

class MonitorWidget: public QWidget
{
  Q_OBJECT
public:
  explicit MonitorWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  virtual ~MonitorWidget();

  void setRefreshInterval(int i);
  void setPort(const QString& p);

protected:
  void refreshTimeout();

private:
  Ui::MonitorWidget* _ui;
  QTimer _timer;
  xdm1041_t _xdm1041;
  QString _oldFunc;
  QLineSeries* _series{nullptr};
  QList<double> _values;
};