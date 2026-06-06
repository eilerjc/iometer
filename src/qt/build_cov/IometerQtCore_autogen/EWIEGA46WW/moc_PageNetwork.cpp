/****************************************************************************
** Meta object code from reading C++ file 'PageNetwork.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../PageNetwork.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PageNetwork.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN11PageNetworkE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN11PageNetworkE = QtMocHelpers::stringData(
    "PageNetwork",
    "clearSelection",
    "",
    "setSelectedManager",
    "mgrName",
    "setSelectedWorker",
    "workerId",
    "refreshForEngine",
    "onManagerConnected",
    "ManagerInfo",
    "mgr",
    "onManagerDisconnected",
    "name",
    "onTargetItemChanged",
    "QTreeWidgetItem*",
    "item",
    "column",
    "onNicComboChanged",
    "index",
    "onMaxSendsChanged",
    "value",
    "onTestConnRateToggled",
    "checked",
    "onTransPerConnChanged"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN11PageNetworkE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x0a,    1 /* Public */,
       3,    1,   81,    2, 0x0a,    2 /* Public */,
       5,    2,   84,    2, 0x0a,    4 /* Public */,
       7,    0,   89,    2, 0x0a,    7 /* Public */,
       8,    1,   90,    2, 0x0a,    8 /* Public */,
      11,    1,   93,    2, 0x0a,   10 /* Public */,
      13,    2,   96,    2, 0x08,   12 /* Private */,
      17,    1,  101,    2, 0x08,   15 /* Private */,
      19,    1,  104,    2, 0x08,   17 /* Private */,
      21,    1,  107,    2, 0x08,   19 /* Private */,
      23,    1,  110,    2, 0x08,   21 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    4,    6,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, 0x80000000 | 14, QMetaType::Int,   15,   16,
    QMetaType::Void, QMetaType::Int,   18,
    QMetaType::Void, QMetaType::Int,   20,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void, QMetaType::Int,   20,

       0        // eod
};

Q_CONSTINIT const QMetaObject PageNetwork::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN11PageNetworkE.offsetsAndSizes,
    qt_meta_data_ZN11PageNetworkE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN11PageNetworkE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PageNetwork, std::true_type>,
        // method 'clearSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setSelectedManager'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setSelectedWorker'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshForEngine'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onManagerConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ManagerInfo &, std::false_type>,
        // method 'onManagerDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onTargetItemChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QTreeWidgetItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onNicComboChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onMaxSendsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onTestConnRateToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onTransPerConnChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void PageNetwork::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PageNetwork *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->clearSelection(); break;
        case 1: _t->setSelectedManager((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->setSelectedWorker((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->refreshForEngine(); break;
        case 4: _t->onManagerConnected((*reinterpret_cast< std::add_pointer_t<ManagerInfo>>(_a[1]))); break;
        case 5: _t->onManagerDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onTargetItemChanged((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->onNicComboChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->onMaxSendsChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->onTestConnRateToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->onTransPerConnChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *PageNetwork::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PageNetwork::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN11PageNetworkE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PageNetwork::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
