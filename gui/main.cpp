#include <QGuiApplication>
#include "window.h"

int main(int argc, char *argv[])
{
  QGuiApplication app(argc, argv);

  // Set OpenGL Version information
  // Note: This format must be set before show() is called.
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setVersion(3, 3);

  // Set the window up
  Window window;
  window.setFormat(format);
  window.resize(QSize(800, 600));
  window.show();

  return app.exec();
}
