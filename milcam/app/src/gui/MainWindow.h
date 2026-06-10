#pragma once
// MainWindow.h — MilCAM Qt Widgets main window (linuxfb, no GL)
// Fullscreen kiosk shell for the SMB Technics CNC panel PC.

#include <QMainWindow>

// Forward declarations — avoid pulling headers into every translation unit
// that includes this header.
class QLabel;
class QToolBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    // Constructs and wires all UI elements. Called once from the constructor so
    // the constructor body stays readable.
    void buildUi();

    // Owned widgets cached for potential future slot connections.
    QToolBar* m_toolbar  = nullptr;
    QLabel*   m_centerLabel = nullptr;
};
