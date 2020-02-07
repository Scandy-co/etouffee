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
#include "ui_mainwindow.h"

#include <QGLContext>
#include <QOpenGLWidget>
#include <QtWidgets>
#include <iostream>

#if LINUX
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <scandy/utilities/FileOps.h>

#include <scandy/core/IScandyCore.h>
#include <scandy/core/MeshExportOptions.h>
#include <scandy/core/ScannerType.h>

using namespace scandy::core;
using namespace scandy::utilities;

std::shared_ptr<scandy::core::IScandyCore> m_roux;
// VTK must be rendered from main user interface thread
QTimer* m_render_timer;
const int m_render_timeout_ms = 25;

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  m_render_timer = nullptr;

  // Set the app title to version info
  std::stringstream title;
  title << "Etouffee + Roux: " << ScandyCore_VERSION_STRING_FULL;
  QCoreApplication::setApplicationName(QString(title.str().c_str()));
  setWindowTitle(QCoreApplication::applicationName());
}

MainWindow::~MainWindow()
{
  if (m_render_timer) {
    m_render_timer->stop();
  }
  delete ui;
  qApp->exit();
}

void
MainWindow::slotExit()
{
  std::cout << "slotExit()" << std::endl;
  qApp->exit();

  if (m_render_timer) {
    m_render_timer->stop();
  }
}

void
MainWindow::slotRender()
{
  for (auto r : m_roux->getVisualizer()->getViewports()) {
    r->render();
  }
  ui->rouxWidget->GetRenderWindow()->Render();
}

void
MainWindow::showEvent(QShowEvent* event)
{
  // We can't setup the Roux with GL till after the window is shown
  setupRoux();
}

void
MainWindow::setupRoux()
{
  // make the vtkRenderWindow
  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  ui->rouxWidget->SetRenderWindow(renderWindow);
  renderWindow->Render();

  QSize window_size = ui->rouxWidget->size();
  int width = window_size.width();
  int height = window_size.height();
  ui->rouxWidget->SetRenderWindow(renderWindow);
  m_roux = IScandyCore::factoryCreate(width, height, ui->rouxWidget);

  // setup timer to update window every 25ms
  // remember vtk must be update in main ui thread
  if (m_render_timer != nullptr) {
    m_render_timer->stop();
    delete m_render_timer;
  }
  m_render_timer = new QTimer(this);
  connect(m_render_timer, SIGNAL(timeout()), this, SLOT(slotRender()));
  m_render_timer->start(m_render_timeout_ms);

  // Read the license from file
  std::string license_path = DEPS_DIR_PATH "/roux_license.txt";
  std::string license_str =
    (char*)FileOps::ReadDataFromFile<uchar>(license_path).data();
  // Remove the new line characters
  license_str.erase(std::remove(license_str.begin(), license_str.end(), '\n'),
                    license_str.end());
  m_roux->setLicense(license_str);
  std::string model_path = DEPS_DIR_PATH "/models/scandy-4.obj";
  std::string texture_path = DEPS_DIR_PATH "/models/scandy-texture-4.png";
  if (FileOps::FileExists(model_path)) {
    m_roux->loadMesh(model_path, texture_path);
  }
}

void
MainWindow::on_initButton_clicked()
{
  std::cout << "on_initButton_clicked" << std::endl;
  ScannerType scanner_type = ScannerType::FILE;
  std::string dir_path = "/tmp/etouffe_input/";
  auto status = m_roux->initializeScanner(scanner_type, dir_path);
  std::cout << "init " << getStatusString(status) << std::endl;
}

void
MainWindow::on_previewButton_clicked()
{
  std::cout << "on_previewButton_clicked" << std::endl;
  auto status = m_roux->startPreview();
  std::cout << "preview " << getStatusString(status) << std::endl;
}

void
MainWindow::on_startButton_clicked()
{
  std::cout << "on_startButton_clicked" << std::endl;
  auto status = m_roux->startScanning();
  std::cout << "start " << getStatusString(status) << std::endl;
}

void
MainWindow::on_stopButton_clicked()
{
  std::cout << "on_stopButton_clicked" << std::endl;
  auto status = m_roux->stopScanning();
  std::cout << "stop " << getStatusString(status) << std::endl;
}

void
MainWindow::on_meshButton_clicked()
{
  std::cout << "on_meshButton_clicked" << std::endl;
  auto status = m_roux->generateMesh();
  std::cout << "mesh " << getStatusString(status) << std::endl;
}

void
MainWindow::on_saveButton_clicked()
{
  std::cout << "on_saveButton_clicked" << std::endl;
  MeshExportOptions opts;
  opts.m_dst_file_path = "/tmp/etouffee.ply";
  auto status = m_roux->exportMesh(opts);
  std::cout << "save " << getStatusString(status) << std::endl;
}
