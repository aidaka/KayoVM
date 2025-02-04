/*
 * Author: kayo
 */

#ifndef JVM_ENCODING_H
#define JVM_ENCODING_H

#include "../jtypes.h"

jchar *utf8_to_unicode(const char *str);

char *unicode_to_utf8(const jchar arr[], size_t len);

#endif //JVM_ENCODING_H
