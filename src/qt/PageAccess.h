// PageAccess.h -- "Access Specifications" tab.
// Layout matches the original:
//   Left:   "Assigned Access Specifications" list + Move Up/Down
//   Center: << Add / Remove >> transfer buttons
//   Right:  "Global Access Specifications" list + New/Edit/Edit Copy/Delete
#pragma once
#include "IometerTypes.h"
#include <QWidget>

class IometerEngine;
class QListWidget;
class QPushButton;
class QDialog;

class PageAccess : public QWidget
{
    Q_OBJECT
public:
    explicit PageAccess(IometerEngine *engine, QWidget *parent = nullptr);

public slots:
    void loadSpecList();   // rebuild both panels from engine state
    void setActiveSpecIndex(int idx);  // highlight the currently-running assigned spec (-1 = none)

    // Returns the AccessSpec objects currently in the Assigned list, in order.
    QList<AccessSpec> currentAssignedSpecs() const;

private slots:
    void onAddToAssigned();
    void onRemoveFromAssigned();
    void onMoveUp();
    void onMoveDown();
    void onNewSpec();
    void onEditSpec();
    void onEditCopySpec();
    void onDeleteSpec();
    void onGlobalSelectionChanged();
    void onAssignedSelectionChanged();

private:
    void setupUi();
    void rebuildGlobalList();
    void rebuildAssignedList();
    bool editSpecDialog(AccessSpec &spec, const QString &title);

    IometerEngine *m_engine    = nullptr;
    QListWidget   *m_assigned  = nullptr;   // left column
    QListWidget   *m_global    = nullptr;   // right column
    QPushButton   *m_addBtn    = nullptr;   // << Add
    QPushButton   *m_removeBtn = nullptr;   // Remove >>
    QPushButton   *m_upBtn     = nullptr;
    QPushButton   *m_downBtn   = nullptr;
    QPushButton   *m_newBtn    = nullptr;
    QPushButton   *m_editBtn   = nullptr;
    QPushButton   *m_copyBtn   = nullptr;
    QPushButton   *m_delBtn    = nullptr;
};
