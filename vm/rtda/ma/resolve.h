/*
 * Author: kayo
 */

#ifndef JVM_RESOLVE_H
#define JVM_RESOLVE_H

#include <cstdint>

class Class;
class Method;
class Field;
class Object;

Class* resolve_class(Class *visitor, int cp_index);

Method* resolve_method(Class *visitor, int cp_index);

Field* resolve_field(Class *visitor, int cp_index);

Object* resolve_string(Class *c, int cp_index);

uintptr_t resolve_single_constant(Class *c, int cp_index);

#endif //JVM_RESOLVE_H
