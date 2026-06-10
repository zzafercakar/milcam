// milcam/app/src/gui/MainWindow.cpp
#include "gui/MainWindow.h"
#include "gui/TopBar.h"
#include "gui/OperationToolBar.h"
#include "gui/CanvasView.h"
#include "gui/JobPanel.h"
#include "gui/StatusBar.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("MilCAM");
    buildUi();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    outer->addWidget(new TopBar(central));        // top

    auto* mid = new QWidget(central);             // middle row
    auto* midLay = new QHBoxLayout(mid);
    midLay->setContentsMargins(0, 0, 0, 0);
    midLay->setSpacing(0);
    midLay->addWidget(new OperationToolBar(mid));
    midLay->addWidget(new CanvasView(mid), 1);    // canvas stretches
    midLay->addWidget(new JobPanel(mid));
    outer->addWidget(mid, 1);

    outer->addWidget(new StatusBar(central));     // bottom

    setCentralWidget(central);
}
