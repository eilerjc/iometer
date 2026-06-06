/****************************************************************************
** Meta object code from reading C++ file 'PageAccess.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../PageAccess.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PageAccess.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10PageAccessE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10PageAccessE = QtMocHelpers::stringData(
    "PageAccess",
    "loadSpecList",
    "",
    "setActiveSpecIndex",
    "idx",
    "currentAssignedSpecs",
    "QList<AccessSpec>",
    "onAddToAssigned",
    "onRemoveFromAssigned",
    "onMoveUp",
    "onMoveDown",
    "onNewSpec",
    "onEditSpec",
    "onEditCopySpec",
    "onDeleteSpec",
    "onGlobalSelectionChanged",
    "onAssignedSelectionChanged"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10PageAccessE[] = {

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
       5,    0,   96,    2, 0x10a,    4 /* Public | MethodIsConst  */,
       7,    0,   97,    2, 0x08,    5 /* Private */,
       8,    0,   98,    2, 0x08,    6 /* Private */,
       9,    0,   99,    2, 0x08,    7 /* Private */,
      10,    0,  100,    2, 0x08,    8 /* Private */,
      11,    0,  101,    2, 0x08,    9 /* Private */,
      12,    0,  102,    2, 0x08,   10 /* Private */,
      13,    0,  103,    2, 0x08,   11 /* Private */,
      14,    0,  104,    2, 0x08,   12 /* Private */,
      15,    0,  105,    2, 0x08,   13 /* Private */,
      16,    0,  106,    2, 0x08,   14 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    0x80000000 | 6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject PageAccess::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN10PageAccessE.offsetsAndSizes,
    qt_meta_data_ZN10PageAccessE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10PageAccessE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PageAccess, std::true_type>,
        // method 'loadSpecList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setActiveSpecIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'currentAssignedSpecs'
        QtPrivate::TypeAndForceComplete<QList<AccessSpec>, std::false_type>,
        // method 'onAddToAssigned'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRemoveFromAssigned'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMoveUp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMoveDown'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onNewSpec'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEditSpec'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEditCopySpec'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteSpec'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGlobalSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAssignedSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PageAccess::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PageAccess *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->loadSpecList(); break;
        case 1: _t->setActiveSpecIndex((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: { QList<AccessSpec> _r = _t->currentAssignedSpecs();
            if (_a[0]) *reinterpret_cast< QList<AccessSpec>*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->onAddToAssigned(); break;
        case 4: _t->onRemoveFromAssigned(); break;
        case 5: _t->onMoveUp(); break;
        case 6: _t->onMoveDown(); break;
        case 7: _t->onNewSpec(); break;
        case 8: _t->onEditSpec(); break;
        case 9: _t->onEditCopySpec(); break;
        case 10: _t->onDeleteSpec(); break;
        case 11: _t->onGlobalSelectionChanged(); break;
        case 12: _t->onAssignedSelectionChanged(); break;
        default: ;
        }
    }
}

const QMetaObject *PageAccess::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PageAccess::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10PageAccessE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PageAccess::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
