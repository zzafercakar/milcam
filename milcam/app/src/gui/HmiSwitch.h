// milcam/app/src/gui/HmiSwitch.h
#pragma once
#include <QWidget>
class QToolButton;

// Fixed top-right switch cluster. The "CNC" button returns to the CodeSYS CNC
// HMI. On this single-framebuffer device the visible app is simply whoever wrote
// /dev/fb0 last, and CodeSYS does not auto-repaint, so "switching back" means
// MilCAM EXITS (qApp->quit) and the CodeSYS side forces a full TargetVisu repaint
// to reclaim the screen. CodeSYS re-launches MilCAM when the operator wants CAM.
// (chvt/VT switching proved unworkable here — see .ai/ENGINEERING_LOG.md.)
class HmiSwitch : public QWidget {
    Q_OBJECT
public:
    explicit HmiSwitch(QWidget* parent = nullptr);
private slots:
    void returnToCnc();
private:
    QToolButton* cncBtn_ = nullptr;
};
