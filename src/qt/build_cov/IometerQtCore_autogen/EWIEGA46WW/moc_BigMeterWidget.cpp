/****************************************************************************
** Meta object code from reading C++ file 'BigMeterWidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../BigMeterWidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BigMeterWidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14BigMeterWidgetE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14BigMeterWidgetE = QtMocHelpers::stringData(
    "BigMeterWidget",
    "startRequested",
    "",
    "stopRequested",
    "nextRequested",
    "resultTypeChanged",
    "resultName",
    "onStartNextClicked",
    "onStopClicked",
    "onWatermarkToggled",
    "checked",
    "onMaxRangeChanged",
    "value",
    "onResultTypeChanged",
    "text"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14BigMeterWidgetE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   68,    2, 0x06,    1 /* Public */,
       3,    0,   69,    2, 0x06,    2 /* Public */,
       4,    0,   70,    2, 0x06,    3 /* Public */,
       5,    1,   71,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   74,    2, 0x08,    6 /* Private */,
       8,    0,   75,    2, 0x08,    7 /* Private */,
       9,    1,   76,    2, 0x08,    8 /* Private */,
      11,    1,   79,    2, 0x08,   10 /* Private */,
      13,    1,   82,    2, 0x08,   12 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::QString,   14,

       0        // eod
};

Q_CONSTINIT const QMetaObject BigMeterWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN14BigMeterWidgetE.offsetsAndSizes,
    qt_meta_data_ZN14BigMeterWidgetE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14BigMeterWidgetE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BigMeterWidget, std::true_type>,
        // method 'startRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'nextRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resultTypeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onStartNextClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStopClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onWatermarkToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onMaxRangeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onResultTypeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void BigMeterWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BigMeterWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->startRequested(); break;
        case 1: _t->stopRequested(); break;
        case 2: _t->nextRequested(); break;
        case 3: _t->resultTypeChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->onStartNextClicked(); break;
        case 5: _t->onStopClicked(); break;
        case 6: _t->onWatermarkToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->onMaxRangeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->onResultTypeChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (BigMeterWidget::*)();
            if (_q_method_type _q_method = &BigMeterWidget::startRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (BigMeterWidget::*)();
            if (_q_method_type _q_method = &BigMeterWidget::stopRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (BigMeterWidget::*)();
            if (_q_method_type _q_method = &BigMeterWidget::nextRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (BigMeterWidget::*)(const QString & );
            if (_q_method_type _q_method = &BigMeterWidget::resultTypeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *BigMeterWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BigMeterWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14BigMeterWidgetE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int BigMeterWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void BigMeterWidget::startRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void BigMeterWidget::stopRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BigMeterWidget::nextRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void BigMeterWidget::resultTypeChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
