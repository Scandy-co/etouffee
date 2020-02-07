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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow* ui;

public slots:
  void slotExit();
  void slotRender();

protected slots:
  void showEvent(QShowEvent* event);
  void setupRoux();
  void on_initButton_clicked();
  void on_previewButton_clicked();
  void on_startButton_clicked();
  void on_stopButton_clicked();
  void on_meshButton_clicked();
  void on_saveButton_clicked();
  void on_scanSize_valueChanged(double arg1);
  void on_voxelSize_valueChanged(double arg1);
  void on_scannerType_currentIndexChanged(int index);
  void on_v2ScanMode_stateChanged(int arg1);
};

#endif // MAINWINDOW_H
