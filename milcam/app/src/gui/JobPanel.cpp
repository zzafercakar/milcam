// milcam/app/src/gui/JobPanel.cpp
#include "gui/JobPanel.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QFormLayout>
#include <QLineEdit>

JobPanel::JobPanel(QWidget* parent) : QWidget(parent) {
    setObjectName("JobPanel");
    setFixedWidth(240);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto* treeHdr = new QLabel("JOB", this);
    treeHdr->setObjectName("PanelHeader");
    lay->addWidget(treeHdr);

    auto* tree = new QTreeWidget(this);
    tree->setHeaderHidden(true);
    tree->setEnabled(false);                 // placeholder
    auto* stock = new QTreeWidgetItem(tree, {"Stock 100 x 80 x 20"});
    new QTreeWidgetItem(stock, {"Profile 1"});
    new QTreeWidgetItem(stock, {"Pocket 1"});
    tree->expandAll();
    lay->addWidget(tree, 1);

    auto* paramHdr = new QLabel("PARAMETERS", this);
    paramHdr->setObjectName("PanelHeader");
    lay->addWidget(paramHdr);

    auto* form = new QWidget(this);
    auto* fl = new QFormLayout(form);
    auto* tool = new QLineEdit("Ø6 flat", form); tool->setEnabled(false);
    auto* depth = new QLineEdit("5.0 mm", form); depth->setEnabled(false);
    auto* feed = new QLineEdit("600 mm/min", form); feed->setEnabled(false);
    fl->addRow("Tool", tool);
    fl->addRow("Depth", depth);
    fl->addRow("Feed", feed);
    lay->addWidget(form);
}
