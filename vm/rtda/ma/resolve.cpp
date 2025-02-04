/*
 * Author: kayo
 */

#include "resolve.h"
#include "Field.h"
#include "../../classfile/constant.h"
#include "../heap/StrPool.h"
#include "ConstantPool.h"
#include "../ma/Class.h"

Class* resolve_class(Class *visitor, int cp_index)
{
    ConstantPool &cp = visitor->cp;
    if (CP_TYPE(cp, cp_index) == CONSTANT_ResolvedClass) {
        return (Class *) CP_INFO(cp, cp_index);
    }

    Class *c = visitor->loader->loadClass(CP_CLASS_NAME(cp, cp_index));
    if (!c->isAccessibleTo(visitor)) {
        raiseException(ILLEGAL_ACCESS_ERROR);
    }

    CP_TYPE(cp, cp_index) = CONSTANT_ResolvedClass;
    CP_INFO(cp, cp_index) = (uintptr_t) c;
    return c;
}

Method* resolve_method(Class *visitor, int cp_index)
{
    ConstantPool &cp = visitor->cp;
    u1 type = CP_TYPE(cp, cp_index);

    if (type == CONSTANT_ResolvedMethod) {
        return (Method *) CP_INFO(cp, cp_index);
    }

    Class *resolved_class = resolve_class(visitor, CP_METHOD_CLASS_INDEX(cp, cp_index));
    Method *m = resolved_class->lookupMethod(CP_METHOD_NAME(cp, cp_index), CP_METHOD_TYPE(cp, cp_index));
    if (!m->isAccessibleTo(visitor)) {
        raiseException(ILLEGAL_ACCESS_ERROR);
    }

    CP_TYPE(cp, cp_index) = CONSTANT_ResolvedMethod;
    CP_INFO(cp, cp_index) = (uintptr_t) m;
    return m;
}

Field* resolve_field(Class *visitor, int cp_index)
{
    ConstantPool &cp = visitor->cp;
    u1 type = CP_TYPE(cp, cp_index);

    if (type == CONSTANT_ResolvedField) {
        return (Field *) CP_INFO(cp, cp_index);
    }

    Class *resolved_class = resolve_class(visitor, CP_FIELD_CLASS_INDEX(cp, cp_index));
    Field *f = resolved_class->lookupField(CP_FIELD_NAME(cp, cp_index), CP_FIELD_TYPE(cp, cp_index));
    if (!f->isAccessibleTo(visitor)) {
        raiseException(ILLEGAL_ACCESS_ERROR);
    }

    CP_TYPE(cp, cp_index) = CONSTANT_ResolvedField;
    CP_INFO(cp, cp_index) = (uintptr_t) f;
    return f;
}

Object* resolve_string(Class *c, int cp_index)
{
    ConstantPool &cp = c->cp;
    if (CP_TYPE(cp, cp_index) == CONSTANT_ResolvedString) {
        return (Object *) CP_INFO(cp, cp_index);
    }

    const char *str = CP_STRING(cp, cp_index);
    Object *so = g_str_pool->get(str);
    CP_INFO(cp, cp_index) = (uintptr_t) so;
    CP_TYPE(cp, cp_index) = CONSTANT_ResolvedString;

    return so;
}

uintptr_t resolve_single_constant(Class *c, int cp_index)
{
    ConstantPool &cp = c->cp;

    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Class: return (uintptr_t) resolve_class(c, cp_index);
        case CONSTANT_String: return (uintptr_t) resolve_string(c, cp_index);
        default: return CP_INFO(cp, cp_index);
    }
}
