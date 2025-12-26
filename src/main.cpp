#include <QApplication>

#include <MonitorWidget.h>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  MonitorWidget w;
  w.setPort("COM17");
  w.setWindowTitle("xdm1041");
  w.show();

  return app.exec();
}