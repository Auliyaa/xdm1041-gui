#include <QApplication>

#include <MonitorWidget.h>
#include <xdm1041.h>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  MonitorWidget w;
  w.setWindowTitle("xdm1041");
  w.show();

  return app.exec();
}