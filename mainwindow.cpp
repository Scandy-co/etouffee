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

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <QFileDialog>
#include <QGLContext>
#include <QOpenGLWidget>
#include <QtWidgets>
#include <iostream>
#include <vtksys/SystemTools.hxx>

#include <scandy/utilities/FileOps.h>

#include <scandy/core/IScandyCore.h>
#include <scandy/core/MeshExportOptions.h>
#include <scandy/core/ScannerType.h>
#include <scandy/core/visualizer/MeshViewport.h>

// These need to be last since Xlib.h #define Status 1
#if LINUX
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#undef Status
#endif

using namespace scandy::core;
using namespace scandy::utilities;

static std::shared_ptr<scandy::core::IScandyCore> m_roux;
static std::shared_ptr<scandy::core::IScandyCoreConfiguration> m_sc_config;
static std::string file_dir = "";

// VTK must be rendered from main user interface thread
static QTimer* m_render_timer;
static const int m_render_timeout_ms = 25;

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
  std::string license_path =
    vtksys::SystemTools::GetCurrentWorkingDirectory() + "/roux_license.txt";
  std::vector<char> license_data =
    FileOps::ReadDataFromFile<char>(license_path);
  // convert the std::vector to a std::string
  std::string license_str(license_data.begin(), license_data.end());
  // Remove the new line characters
  license_str.erase(std::remove(license_str.begin(), license_str.end(), '\n'),
                    license_str.end());
  std::cout << "Setting license from: " << license_path << ":\n\t"
            << license_str << std::endl;
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

  IScandyCoreConfiguration::Units u = (IScandyCoreConfiguration::Units)0;
  while (u != IScandyCoreConfiguration::Units::OTHER) {
    QString unit = QString(IScandyCoreConfiguration::GetUnitString(u).c_str());
    ui->sensorUnits->addItem(unit);
    ui->scanningUnits->addItem(unit);
    u = (IScandyCoreConfiguration::Units)((int)u + 1);
  }

  ui->sensorUnits->setCurrentIndex((int)IScandyCoreConfiguration::Units::M);
  ui->scanningUnits->setCurrentIndex((int)IScandyCoreConfiguration::Units::M);

  // Set the default output path to $HOME/Etouffee
  m_sc_config->m_scan_dir_path = getenv("HOME") + std::string("/Etouffee");
  FileOps::EnsureDirectory(m_sc_config->m_scan_dir_path);

  // Load the default sc_config from $HOME/Etouffee/.sc_config.json
  std::string default_sc_config_path = m_sc_config->m_scan_dir_path;
  default_sc_config_path += "/sc_config.json";
  auto sc_config = m_roux->loadIScandyCoreConfiguration(default_sc_config_path);
  if (sc_config) {
    m_sc_config = sc_config;
    // TODO: populate ui with values loaded from config
    // ui->ray
  }

  // TODO: Update this to a timestamped entry
  m_sc_config->m_scan_dir_path = getenv("HOME") + std::string("/Etouffee");
  ui->outputDirectory->setText(QString(m_sc_config->m_scan_dir_path.c_str()));
}

void
MainWindow::on_initButton_clicked()
{
  m_sc_config->m_use_unbounded =
    ui->v2ScanMode->checkState() == Qt::CheckState::Checked;

  std::cout << "on_initButton_clicked: "
            << getScannerTypeString(m_sc_config->m_scanner_type) << " "
            << file_dir << std::endl;

  // Roux currently resets these on init
  const float far = m_sc_config->getFarPlane();
  const float near = m_sc_config->getNearPlane();
  auto status =
    m_roux->initializeScanner(m_sc_config->m_scanner_type, file_dir);
  std::cout << "init " << getStatusString(status) << std::endl;

  m_sc_config->setFarPlane(far);
  m_sc_config->setNearPlane(near);
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
  // Make sure we can save our output to the dir we are outputting to
  FileOps::EnsureDirectory(m_sc_config->m_scan_dir_path);
  auto status = m_roux->startScanning();
  std::stringstream sc_config_path;
  auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  sc_config_path << m_sc_config->m_scan_dir_path << "/sc_config." << epoch
                 << ".json";
  // Save the current scan configuration for future reference
  IScandyCoreConfiguration::SaveToPath(m_sc_config, sc_config_path.str());
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
  // SceneLight works for Qt. Our default SetLightTypeToCameraLight does not
  // seem to work well
  //  m_roux->getMeshViewport()->m_light->SetLightTypeToSceneLight();
  std::cout << "mesh " << getStatusString(status) << std::endl;
}

void
MainWindow::on_saveButton_clicked()
{
  std::cout << "on_saveButton_clicked" << std::endl;
  MeshExportOptions opts;

  // Get the file path from the user via dialog
  QString file_path = QFileDialog::getSaveFileName();
  opts.m_dst_file_path = file_path.toStdString();

  auto status = m_roux->exportMesh(opts);
  std::cout << "save " << getStatusString(status) << std::endl;
}

void
MainWindow::on_scanSize_valueChanged(double arg1)
{
  m_roux->setScanSize((float)arg1, false);
}

void
MainWindow::on_voxelSize_valueChanged(double arg1)
{
  m_roux->setVoxelSize((float)(arg1), false);
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
    QString dir = QFileDialog::getExistingDirectory(
      this,
      tr("Open Directory"),
      QString(m_sc_config->m_scan_dir_path.c_str()),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    file_dir = dir.toStdString();
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
  m_sc_config->setNearPlane((float)arg1);
}

void
MainWindow::on_raycastFarPlane_valueChanged(double arg1)
{
  m_sc_config->setFarPlane((float)arg1);
}

void
MainWindow::on_selectDirectory_clicked()
{
  QString startDir = QString(m_sc_config->m_scan_dir_path.c_str());
  QString dir = QFileDialog::getExistingDirectory(
    this,
    tr("Open Directory"),
    startDir,
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  m_sc_config->m_scan_dir_path = dir.toStdString();
}

void
MainWindow::on_saveInput_stateChanged(int arg1)
{
  m_sc_config->m_save_input_images =
    ui->saveInput->checkState() == Qt::CheckState::Checked;
}

void
MainWindow::on_sensorUnits_activated(int index)
{
  IScandyCoreConfiguration::Units sensorUnits =
    (IScandyCoreConfiguration::Units)ui->sensorUnits->currentIndex();
  IScandyCoreConfiguration::Units scanningUnits =
    (IScandyCoreConfiguration::Units)ui->scanningUnits->currentIndex();
  m_sc_config->setUnits(sensorUnits, scanningUnits);
}

void
MainWindow::on_scanningUnits_activated(int index)
{
  IScandyCoreConfiguration::Units sensorUnits =
    (IScandyCoreConfiguration::Units)ui->sensorUnits->currentIndex();
  IScandyCoreConfiguration::Units scanningUnits =
    (IScandyCoreConfiguration::Units)ui->scanningUnits->currentIndex();
  m_sc_config->setUnits(sensorUnits, scanningUnits);
}
