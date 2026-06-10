// milcam/app/src/gui/HmiSwitch.h
#pragma once
#include <QWidget>
class QToolButton;

// Fixed top-right switch cluster. The HMI button activates the CodeSYS VT so the
// CodeSYS CNC HMI comes to the foreground. Return-to-MilCAM is a CodeSYS-side
// button (chvt to MilCAM's VT) — see the design spec §5.
class HmiSwitch : public QWidget {
    Q_OBJECT
public:
    explicit HmiSwitch(QWidget* parent = nullptr);
private slots:
    void switchToCodesys();
private:
    QToolButton* hmiBtn_ = nullptr;
};
