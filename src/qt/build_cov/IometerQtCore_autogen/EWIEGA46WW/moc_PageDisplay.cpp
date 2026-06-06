/****************************************************************************
** Meta object code from reading C++ file 'PageDisplay.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../PageDisplay.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PageDisplay.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11PageDisplayE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN11PageDisplayE = QtMocHelpers::stringData(
    "PageDisplay",
    "nextSpecRequested",
    "",
    "updateResults",
    "QList<WorkerResult>",
    "results",
    "onTestStarted",
    "onTestStopped",
    "onBigMeterClicked",
    "refreshWorkerAssignments",
    "onMetricButtonClicked",
    "row",
    "onMetricRowClicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN11PageDisplayE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    1,   63,    2, 0x0a,    2 /* Public */,
       6,    0,   66,    2, 0x0a,    4 /* Public */,
       7,    0,   67,    2, 0x0a,    5 /* Public */,
       8,    0,   68,    2, 0x0a,    6 /* Public */,
       9,    0,   69,    2, 0x0a,    7 /* Public */,
      10,    1,   70,    2, 0x08,    8 /* Private */,
      12,    1,   73,    2, 0x08,   10 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void, QMetaType::Int,   11,

       0        // eod
};

Q_CONSTINIT const QMetaObject PageDisplay::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN11PageDisplayE.offsetsAndSizes,
    qt_meta_data_ZN11PageDisplayE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN11PageDisplayE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PageDisplay, std::true_type>,
        // method 'nextSpecRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateResults'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<WorkerResult> &, std::false_type>,
        // method 'onTestStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTestStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBigMeterClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshWorkerAssignments'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMetricButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onMetricRowClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void PageDisplay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PageDisplay *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->nextSpecRequested(); break;
        case 1: _t->updateResults((*reinterpret_cast< std::add_pointer_t<QList<WorkerResult>>>(_a[1]))); break;
        case 2: _t->onTestStarted(); break;
        case 3: _t->onTestStopped(); break;
        case 4: _t->onBigMeterClicked(); break;
        case 5: _t->refreshWorkerAssignments(); break;
        case 6: _t->onMetricButtonClicked((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->onMetricRowClicked((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PageDisplay::*)();
            if (_q_method_type _q_method = &PageDisplay::nextSpecRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *PageDisplay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PageDisplay::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN11PageDisplayE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PageDisplay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void PageDisplay::nextSpecRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
