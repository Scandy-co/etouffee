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

#include <QFileDialog>
#include <QGLContext>
#include <QOpenGLWidget>
#include <QtWidgets>
#include <iostream>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <scandy/utilities/FileOps.h>

#include <scandy/core/IScandyCore.h>
#include <scandy/core/MeshExportOptions.h>
#include <scandy/core/ScannerType.h>


// These need to be last since Xlib.h #define Status 1
#if LINUX
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#undef Status
#endif

using namespace scandy::core;
using namespace scandy::utilities;

std::shared_ptr<scandy::core::IScandyCore> m_roux;
std::shared_ptr<scandy::core::IScandyCoreConfiguration> m_sc_config;

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

  m_sc_config = m_roux->getIScandyCoreConfiguration();

  std::string default_sc_config_path = getenv("HOME");
  default_sc_config_path += "/.sc_config.json";
  auto sc_config = m_roux->loadIScandyCoreConfiguration(default_sc_config_path);
  if( sc_config ){
    m_sc_config = sc_config;
    // TODO: populate ui with values loaded from config
    // ui->ray
  }

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
  license_str.shrink_to_fit();
  std::cout << "Setting license to: " << license_path << ":\n\t" << license_str
            << std::endl;
  auto status = m_roux->setLicense(license_str);
  if (status != scandy::core::Status::SUCCESS) {
    std::cerr << "Error setting Roux License!" << getStatusString(status)
              << std::endl;
    exit((int)Status::INVALID_LICENSE);
  }
  std::string model_path = DEPS_DIR_PATH "/models/scandy-4.obj";
  std::string texture_path = DEPS_DIR_PATH "/models/scandy-texture-4.png";
  if (FileOps::FileExists(model_path)) {
    m_roux->loadMesh(model_path, texture_path);
  }

  // Setup our ScannerType options
  ScannerType s = ScannerType::UNKNOWN;
  while (s != ScannerType::LAST) {
    ui->scannerType->addItem(QString(getScannerTypeString(s)));
    s = (ScannerType)((int)s + 1);
  }
}

void
MainWindow::on_initButton_clicked()
{
  std::cout << "on_initButton_clicked" << std::endl;
  std::string dir_path = m_sc_config->m_scan_dir_path;
  m_sc_config->m_scan_dir_path = "/tmp/etouffe";

  m_sc_config->m_use_unbounded =
    ui->v2ScanMode->checkState() == Qt::CheckState::Checked;

  auto status = m_roux->initializeScanner(m_sc_config->m_scanner_type, dir_path);
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
  // Save the current scan configuration for future reference
  IScandyCoreConfiguration::SaveToDir(m_sc_config);
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
  m_roux->smoothMesh(3);
  m_roux->reverseNormals(true);
  m_roux->applyEditsFromMeshViewport(true);
  std::cout << "mesh " << getStatusString(status) << std::endl;
}

void
MainWindow::on_saveButton_clicked()
{
  std::cout << "on_saveButton_clicked" << std::endl;
  MeshExportOptions opts;
  // TODO: allow setting of this through the UI
  opts.m_dst_file_path = "/tmp/etouffee.ply";
  opts.m_decimate = 0.1;
  opts.m_smoothing = 3;
  auto status = m_roux->exportMesh(opts);
  std::cout << "save " << getStatusString(status) << std::endl;
}

void
MainWindow::on_scanSize_valueChanged(double arg1)
{
  m_roux->setScanSize((float)arg1);
}

void
MainWindow::on_voxelSize_valueChanged(double arg1)
{
  m_roux->setVoxelSize((float)arg1 * 1e-3);
}

void
MainWindow::on_scannerType_currentIndexChanged(int index)
{
  std::cout
    << "on_scannerType_currentIndexChanged: "
    << ui->scannerType->itemText(ui->scannerType->currentIndex()).toStdString()
    << std::endl;
  m_sc_config->m_scanner_type = (ScannerType)ui->scannerType->currentIndex();
  if (m_sc_config->m_scanner_type == ScannerType::FILE) {
    std::string home_dir = getenv("HOME");
    QString dir = QFileDialog::getExistingDirectory(
      this,
      tr("Open Directory"),
      QString(home_dir.c_str()),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    m_sc_config->m_scan_dir_path = dir.toStdString();
  }
}

void
MainWindow::on_v2ScanMode_stateChanged(int arg1)
{
  m_sc_config->m_use_unbounded =
    ui->v2ScanMode->checkState() == Qt::CheckState::Checked;
}

void
MainWindow::on_normalThresh_valueChanged(double arg1)
{
  m_sc_config->m_normal_threshold = (float)arg1;
}

void
MainWindow::on_distanceThresh_valueChanged(double arg1)
{
  m_sc_config->m_dist_threshold = (float)arg1;
}

void
MainWindow::on_raycastNearPlane_valueChanged(double arg1)
{
  m_sc_config->m_raycast_near_plane = (float)arg1;
}

void
MainWindow::on_raycastFarPlane_valueChanged(double arg1)
{
  m_sc_config->m_raycast_far_plane = (float)arg1;
}
