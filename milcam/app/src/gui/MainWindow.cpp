// MainWindow.cpp — MilCAM Qt Widgets main window implementation
// Target: Qt 5.12.4, linuxfb (no GL), aarch64 panel PC @ 1024x768.
// All touch targets are ≥48 px per MilCAM UX requirements.

#include "MainWindow.h"

#include <QAction>
#include <QFont>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>

// ---------------------------------------------------------------------------
// Dark / accent stylesheet
// All colours chosen for visibility on the industrial panel's TFT display.
// Background: near-black (#1A1A2E), toolbar accent: SMB blue (#0F3460),
// highlight: teal (#16213E), text: off-white (#E0E0E0).
// ---------------------------------------------------------------------------
static const char* kStyleSheet = R"(
QMainWindow, QWidget {
    background-color: #1A1A2E;
    color: #E0E0E0;
}

QToolBar {
    background-color: #0F3460;
    border-bottom: 2px solid #E94560;
    spacing: 8px;
    padding: 4px 8px;
}

QToolButton {
    background-color: #16213E;
    color: #E0E0E0;
    border: 1px solid #E94560;
    border-radius: 6px;
    font-size: 15px;
    font-weight: bold;
    min-width:  96px;
    min-height: 48px;
    padding: 4px 12px;
}

QToolButton:disabled {
    background-color: #0F3460;
    color: #607080;
    border-color: #304060;
}

QToolButton:pressed {
    background-color: #E94560;
    color: #FFFFFF;
}

QLabel#centerLabel {
    color: #E0E0E0;
    background-color: transparent;
}
)";

// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
}

void MainWindow::buildUi()
{
    setWindowTitle("MilCAM");

    // Apply dark industrial stylesheet to the whole window.
    setStyleSheet(kStyleSheet);

    // ------------------------------------------------------------------
    // Toolbar — touch-friendly (minimum height 56 px, icon size 48×48).
    // Buttons are visual placeholders; no behaviour wired in Phase 0.
    // ------------------------------------------------------------------
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setMinimumHeight(56);
    m_toolbar->setIconSize(QSize(48, 48));

    // "Open" — will open a CAM job file in a later phase.
    auto* btnOpen = new QToolButton(this);
    btnOpen->setText("Open");
    btnOpen->setMinimumSize(96, 48);
    btnOpen->setEnabled(false); // placeholder: no behaviour yet
    m_toolbar->addWidget(btnOpen);

    m_toolbar->addSeparator();

    // "HMI" — will raise TargetVisu (CODESYS HMI window) via wmctrl.
    auto* btnHmi = new QToolButton(this);
    btnHmi->setText("HMI");
    btnHmi->setMinimumSize(96, 48);
    btnHmi->setEnabled(false); // placeholder: no behaviour yet
    m_toolbar->addWidget(btnHmi);

    addToolBar(Qt::TopToolBarArea, m_toolbar);

    // ------------------------------------------------------------------
    // Central label — confirms the app started; readable at arm's length
    // on the 1024×768 industrial panel.
    // ------------------------------------------------------------------
    m_centerLabel = new QLabel("MilCAM\nSMB Technics 2.5D CAM", this);
    m_centerLabel->setObjectName("centerLabel"); // matched by stylesheet
    m_centerLabel->setAlignment(Qt::AlignCenter);

    QFont f = m_centerLabel->font();
    f.setPointSize(28);
    f.setBold(true);
    m_centerLabel->setFont(f);

    setCentralWidget(m_centerLabel);
}
