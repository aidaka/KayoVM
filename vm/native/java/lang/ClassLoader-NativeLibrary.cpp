/*
 * Author: kayo
 */

#include "../../registry.h"
#include "../../../kayo.h"
#include "../../../rtda/thread/Frame.h"
#include "../../../slot.h"
#include "../../../rtda/heap/Object.h"

// native long find(String name);
static void find(Frame *frame)
{
    // todo
//    jref name = slot_getr(frame->local_vars + 1);
//    printvm("-------- %s\n", jstrobj_value(name));
    frame->pushl(1); // todo
}

// native void load(String name, boolean isBuiltin);
static void load(Frame *frame)
{
    // todo
    jref _this = frame->getLocalAsRef(0);
    jref name = frame->getLocalAsRef(1);
//    bool is_builtin = slot_geti(frame->local_vars + 2) == 0 ? false : true;

    // todo load

    // set boolean loaded is true
    _this->setFieldValue("loaded", "Z", 1);
}

// native void unload(String name, boolean isBuiltin);
static void unload(Frame *frame)
{

}

void java_lang_ClassLoader$NativeLibrary_registerNatives()
{
    register_native_method("java/lang/ClassLoader$NativeLibrary", "find", "(Ljava/lang/String;)J", find);
    register_native_method("java/lang/ClassLoader$NativeLibrary", "load", "(Ljava/lang/String;Z)V", load);
    register_native_method("java/lang/ClassLoader$NativeLibrary", "unload", "(Ljava/lang/String;Z)V", unload);
}

