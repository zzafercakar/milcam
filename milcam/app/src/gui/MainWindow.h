// milcam/app/src/gui/MainWindow.h
#pragma once
#include <QMainWindow>

// Top-level shell: TopBar, left OperationToolBar, center CanvasView, right
// JobPanel, bottom StatusBar. Composition only; sub-widgets own their content.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private:
    void buildUi();
};
