// milcam/app/src/gui/TopBar.h
#pragma once
#include <QWidget>

// Top bar: brand (left), primary actions (center), fixed switch cluster (right).
class TopBar : public QWidget {
    Q_OBJECT
public:
    explicit TopBar(QWidget* parent = nullptr);
};
