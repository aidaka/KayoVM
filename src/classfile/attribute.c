/*
 * Author: Jia Yang
 */

#include <string.h>
#include <assert.h>
#include "../jvm.h"
#include "constant.h"
#include "../util/encoding.h"
#include "attribute.h"

void read_annotation(struct bytecode_reader *reader, struct annotation *a)
{
    assert(reader != NULL);
    assert(a != NULL);

    a->type_index = bcr_readu2(reader);
    a->num_element_value_pairs = bcr_readu2(reader);
    a->element_value_pairs = malloc(sizeof(struct element_value_pair) * a->num_element_value_pairs);
    CHECK_MALLOC_RESULT(a->element_value_pairs);
    for (int i = 0; i < a->num_element_value_pairs; i++) {
        a->element_value_pairs[i].element_name_index = bcr_readu2(reader);
        a->element_value_pairs[i].value = malloc(sizeof(struct element_value));
        CHECK_MALLOC_RESULT(a->element_value_pairs[i].value);
        read_element_value(reader, a->element_value_pairs[i].value);
    }
}

void read_element_value(struct bytecode_reader *reader, struct element_value *ev)
{
    assert(reader != NULL);
    assert(ev != NULL);

    ev->tag = bcr_readu1(reader);
    switch (ev->tag) {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'S':
        case 'Z':
        case 's':
            ev->value.const_value_index = bcr_readu2(reader);
            break;
        case 'c':
            ev->value.class_info_index = bcr_readu2(reader);
            break;
        case 'e':
            ev->value.enum_const_value.type_name_index = bcr_readu2(reader);
            ev->value.enum_const_value.const_name_index = bcr_readu2(reader);
            break;
        case '@':
            read_annotation(reader, &(ev->value.annotation_value));
            break;
        case '[':
            ev->value.array_value.num_values = bcr_readu2(reader);
            ev->value.array_value.values = malloc(sizeof(struct element_value) * ev->value.array_value.num_values);
            CHECK_MALLOC_RESULT(ev->value.array_value.values);
            for (int i = 0; i < ev->value.array_value.num_values; i++) {
                read_element_value(reader, ev->value.array_value.values + i);
            }
            break;
        default:
            VM_UNKNOWN_ERROR("unknown tag: %d", ev->tag);
    }
}
