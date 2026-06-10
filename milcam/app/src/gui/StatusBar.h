// milcam/app/src/gui/StatusBar.h
#pragma once
#include <QWidget>

// Bottom bar: units, cursor coords, post status, and the SEND G-CODE button.
class StatusBar : public QWidget {
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);
};
