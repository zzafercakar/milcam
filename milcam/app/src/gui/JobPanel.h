// milcam/app/src/gui/JobPanel.h
#pragma once
#include <QWidget>

// Right dock: job tree (Stock + operations) on top, parameter form below.
// Static sample content now; populated by the document model in P1/P3.
class JobPanel : public QWidget {
    Q_OBJECT
public:
    explicit JobPanel(QWidget* parent = nullptr);
};
