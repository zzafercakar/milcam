// milcam/app/src/gui/OperationToolBar.cpp
#include "gui/OperationToolBar.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QIcon>

namespace {
QToolButton* tool(QWidget* parent, const QString& icon, const QString& tip) {
    auto* b = new QToolButton(parent);
    b->setIcon(QIcon(":/icons/" + icon + ".svg"));
    b->setIconSize(QSize(34, 34));
    b->setMinimumSize(60, 60);
    b->setToolTip(tip);
    b->setEnabled(false);   // placeholder until P1/P3
    return b;
}
}  // namespace

OperationToolBar::OperationToolBar(QWidget* parent) : QWidget(parent) {
    setObjectName("OperationToolBar");
    setFixedWidth(72);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 8, 6, 8);
    lay->setSpacing(6);
    lay->addWidget(tool(this, "dxf", "Import DXF"));
    lay->addWidget(tool(this, "rect", "Rectangle"));
    lay->addWidget(tool(this, "circle", "Circle"));
    lay->addWidget(tool(this, "line", "Line"));
    lay->addSpacing(12);
    lay->addWidget(tool(this, "profile", "Profile"));
    lay->addWidget(tool(this, "pocket", "Pocket"));
    lay->addWidget(tool(this, "drill", "Drill"));
    lay->addStretch(1);
}
