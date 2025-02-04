/*
 * Author: kayo
 */

#include <sstream>
#include "../ma/Class.h"
#include "Object.h"
#include "../ma/Field.h"
#include "../../symbol.h"
#include "StringObject.h"

using namespace std;

Object *Object::newInst(Class *c)
{
    if (c == java_lang_String) {
        return StringObject::newInst(""); // todo
    }
    size_t size = sizeof(Object) + c->instFieldsCount * sizeof(slot_t);
    return new(g_heap_mgr.get(size)) Object(c);
}

void Object::operator delete(void *rawMemory,std::size_t size) throw()
{
    // todo 将内存返回 g_heap_mgr
}

Object *Object::clone() const
{
    size_t s = size();
    return (Object *) memcpy(g_heap_mgr.get(s), this, s);
}

void Object::setFieldValue(Field *f, slot_t v)
{
    assert(f != nullptr);
    assert(!f->isStatic());

    if (!f->categoryTwo) {
        data[f->id] = v;
    } else { // categoryTwo
        data[f->id] = 0; // 高字节清零
        data[f->id + 1] = v; // 低字节存值
    }
}

void Object::setFieldValue(Field *f, const slot_t *value)
{
    assert(f != nullptr && !f->isStatic() && value != nullptr);

    data[f->id] = value[0];
    if (f->categoryTwo) {
        data[f->id + 1] = value[1];
    }
}

void Object::setFieldValue(const char *name, const char *descriptor, slot_t v)
{
    assert(name != nullptr && descriptor != nullptr);
    setFieldValue(clazz->lookupInstField(name, descriptor), v);
}

void Object::setFieldValue(const char *name, const char *descriptor, const slot_t *value)
{
    assert(name != nullptr && descriptor != nullptr && value != nullptr);
    setFieldValue(clazz->lookupInstField(name, descriptor), value);
}

const slot_t *Object::getInstFieldValue0(const char *name, const char *descriptor) const
{
    assert(name != nullptr && descriptor != nullptr);

    Field *f = clazz->lookupField(name, descriptor);
    if (f == nullptr) {
        jvm_abort("error, %s, %s\n", name, descriptor); // todo
    }

    return data + f->id;
}

bool Object::isInstanceOf(const Class *c) const
{
    if (c == nullptr)  // todo
        return false;
    return clazz->isSubclassOf(c);
}

const slot_t *Object::unbox() const
{
    if (clazz->isPrimitive()) {
        // value 的描述符就是基本类型的类名。比如，private final boolean value;
        Field *f = clazz->lookupField(S(value), clazz->className);
        if (f == nullptr) {
            jvm_abort("error, %s, %s\n", S(value), clazz->className); // todo
        }
        return data + f->id;
//        return getInstFieldValue(S(value), clazz->class_name);
    }
    // todo error!!!!!!!!
    jvm_abort("error");
}

size_t Object::size() const
{
    return sizeof(*this) + clazz->instFieldsCount * sizeof(slot_t);
}

bool Object::isArray() const
{
    return clazz->isArray();
}

string Object::toString() const
{
    ostringstream os;
    os << "Object(" << this << "), " << clazz->className;
    return os.str();
}
