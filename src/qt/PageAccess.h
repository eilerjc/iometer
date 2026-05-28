// PageAccess.h — "Access Specifications" tab.
// Equivalent to CPageAccess in the MFC GUI.
#pragma once
#include "IometerTypes.h"
#include <QWidget>
class IometerEngine;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QGroupBox;

class PageAccess : public QWidget
{
    Q_OBJECT
public:
    explicit PageAccess(IometerEngine *engine, QWidget *parent = nullptr);

private slots:
    void onSpecSelected(QListWidgetItem *item);
    void onAddSpec();
    void onRemoveSpec();
    void onDuplicateSpec();
    void onSpecEdited();
    void onMoveUp();
    void onMoveDown();

private:
    void setupUi();
    void loadSpecList();
    void saveCurrentSpec();
    void loadSpecEditor(int index);
    void populateXferSizes(QComboBox *cb);

    IometerEngine  *m_engine      = nullptr;
    QListWidget    *m_specList    = nullptr;
    QPushButton    *m_addBtn      = nullptr;
    QPushButton    *m_removeBtn   = nullptr;
    QPushButton    *m_dupBtn      = nullptr;
    QPushButton    *m_upBtn       = nullptr;
    QPushButton    *m_downBtn     = nullptr;

    // Editor controls
    QLineEdit      *m_nameEdit    = nullptr;
    QComboBox      *m_xferSize    = nullptr;
    QComboBox      *m_alignSize   = nullptr;
    QSpinBox       *m_readPct     = nullptr;
    QSpinBox       *m_seqPct      = nullptr;
    QSpinBox       *m_burstLen    = nullptr;
    QDoubleSpinBox *m_delayMs     = nullptr;
    QSpinBox       *m_iterations  = nullptr;
    QCheckBox      *m_defaultSpec = nullptr;
    QLabel         *m_descLabel   = nullptr;

    int  m_currentIndex = -1;
    bool m_updating     = false;
};
