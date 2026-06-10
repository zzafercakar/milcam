// milcam/app/src/gui/OperationToolBar.h
#pragma once
#include <QWidget>

// Left vertical strip of CAM operation/geometry tools (icons). Placeholder
// buttons (disabled) until DXF import / shapes / ops land in P1/P3.
class OperationToolBar : public QWidget {
    Q_OBJECT
public:
    explicit OperationToolBar(QWidget* parent = nullptr);
};
