/****************************************************************************
** Meta object code from reading C++ file 'sqlworker.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../sqlworker/sqlworker.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sqlworker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SqlWorker[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   11,   10,   10, 0x05,
      38,   10,   10,   10, 0x05,
      53,   10,   10,   10, 0x05,
      67,   61,   10,   10, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_SqlWorker[] = {
    "SqlWorker\0\0errorString\0error(QString)\0"
    "adminUpdated()\0ready()\0error\0"
    "error(SqlError)\0"
};

void SqlWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SqlWorker *_t = static_cast<SqlWorker *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->adminUpdated(); break;
        case 2: _t->ready(); break;
        case 3: _t->error((*reinterpret_cast< const SqlError(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SqlWorker::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SqlWorker::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SqlWorker,
      qt_meta_data_SqlWorker, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SqlWorker::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SqlWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SqlWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SqlWorker))
        return static_cast<void*>(const_cast< SqlWorker*>(this));
    return QObject::qt_metacast(_clname);
}

int SqlWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void SqlWorker::error(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SqlWorker::adminUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void SqlWorker::ready()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void SqlWorker::error(const SqlError _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
