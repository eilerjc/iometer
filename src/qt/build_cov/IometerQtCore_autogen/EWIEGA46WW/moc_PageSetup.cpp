/****************************************************************************
** Meta object code from reading C++ file 'PageSetup.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../PageSetup.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PageSetup.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9PageSetupE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN9PageSetupE = QtMocHelpers::stringData(
    "PageSetup",
    "clearSelection",
    "",
    "setSelectedManager",
    "mgrName",
    "setSelectedWorker",
    "workerId",
    "refreshForEngine",
    "onTargetItemChanged",
    "QListWidgetItem*",
    "item",
    "onQueueDepthChanged",
    "value",
    "onMaxDiskSizeChanged",
    "onStartSectorChanged",
    "onFixedSeedToggled",
    "checked",
    "onFixedSeedValueChanged",
    "onConnRateToggled",
    "onTransPerConnChanged",
    "onDataPatternChanged",
    "index"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN9PageSetupE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   92,    2, 0x0a,    1 /* Public */,
       3,    1,   93,    2, 0x0a,    2 /* Public */,
       5,    2,   96,    2, 0x0a,    4 /* Public */,
       7,    0,  101,    2, 0x0a,    7 /* Public */,
       8,    1,  102,    2, 0x08,    8 /* Private */,
      11,    1,  105,    2, 0x08,   10 /* Private */,
      13,    1,  108,    2, 0x08,   12 /* Private */,
      14,    1,  111,    2, 0x08,   14 /* Private */,
      15,    1,  114,    2, 0x08,   16 /* Private */,
      17,    0,  117,    2, 0x08,   18 /* Private */,
      18,    1,  118,    2, 0x08,   19 /* Private */,
      19,    1,  121,    2, 0x08,   21 /* Private */,
      20,    1,  124,    2, 0x08,   23 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    4,    6,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Bool,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   16,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Int,   21,

       0        // eod
};

Q_CONSTINIT const QMetaObject PageSetup::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN9PageSetupE.offsetsAndSizes,
    qt_meta_data_ZN9PageSetupE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN9PageSetupE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PageSetup, std::true_type>,
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
        // method 'onTargetItemChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>,
        // method 'onQueueDepthChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onMaxDiskSizeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onStartSectorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onFixedSeedToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onFixedSeedValueChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConnRateToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onTransPerConnChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onDataPatternChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void PageSetup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PageSetup *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->clearSelection(); break;
        case 1: _t->setSelectedManager((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->setSelectedWorker((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->refreshForEngine(); break;
        case 4: _t->onTargetItemChanged((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 5: _t->onQueueDepthChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->onMaxDiskSizeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->onStartSectorChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->onFixedSeedToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 9: _t->onFixedSeedValueChanged(); break;
        case 10: _t->onConnRateToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->onTransPerConnChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->onDataPatternChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *PageSetup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PageSetup::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN9PageSetupE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PageSetup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
