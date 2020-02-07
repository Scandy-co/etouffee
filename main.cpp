/****************************************************************************\
 * Copyright (C) 2014-2020 Scandy
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

// For distribution.

#include <scandy.h>

#include <scandy_version.h>
// ^^ Always include this first. It #defines lots of things we need throughout

#include "mainwindow.h"

#include <QVTKOpenGLNativeWidget.h>

#include <QApplication>
#include <QGLContext>

int
main(int argc, char* argv[])
{
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  QCoreApplication::setOrganizationName("Scandy");
  QCoreApplication::setApplicationName("Etouffee");
  QCoreApplication::setApplicationVersion(ScandyCore_VERSION_STRING_FULL);

  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  return a.exec();
}
