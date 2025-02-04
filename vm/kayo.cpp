/*
 * Author: kayo
 */

#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <iostream>
#include <typeinfo>
#include "kayo.h"
#include "debug.h"
#include "loader/ClassLoader.h"
#include "rtda/thread/Thread.h"
#include "rtda/ma/Class.h"
#include "interpreter/interpreter.h"
#include "rtda/heap/StrPool.h"
#include "native/registry.h"

using namespace std;

#if TRACE_KAYO
#define TRACE PRINT_TRACE
#else
#define TRACE(...)
#endif

HeapMgr g_heap_mgr;

vector<std::string> jreLibJars;
vector<std::string> jreExtJars;
vector<std::string> userDirs;
vector<std::string> userJars;

StrPool *g_str_pool;

Object *sysThreadGroup;

vector<Thread *> g_all_threads;

void init_symbol();

static void *gcLoop(void *arg)
{
    // todo
    return nullptr;
}

static void findJars(const char *path, vector<std::string> &result)
{
    DIR *dir = opendir(path);
    if (dir == nullptr) {
        printvm("open dir failed. %s\n", path);
    }

    struct dirent *entry;
    struct stat statbuf;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char abspath[PATH_MAX];
        // sprintf 和 snprintf 会自动在加上字符串结束符'\0'
        sprintf(abspath, "%s/%s", path, entry->d_name); // 绝对路径

        stat(abspath, &statbuf);
        if (S_ISREG(statbuf.st_mode)) { // 常规文件
            char *suffix = strrchr(abspath, '.');
            if (suffix != nullptr && strcmp(suffix, ".jar") == 0)
                result.emplace_back(abspath);
        }
    }

    closedir(dir);
}

static char main_class_name[FILENAME_MAX] = { 0 };

void initJVM(int argc, char* argv[])
{
//    time_t time1;
//    time(&time1);

    char bootstrap_classpath[PATH_MAX] = { 0 };
    char extension_classpath[PATH_MAX] = { 0 };
    char user_classpath[PATH_MAX] = { 0 };

    // parse cmd arguments
    // 可执行程序的名字为 argv[0]，跳过。
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            const char *name = argv[i];
            if (strcmp(name, "-bcp") == 0) { // parse Bootstrap Class Path
                if (++i >= argc) {
                    jvm_abort("缺少参数：%s\n", name);
                }
                strcpy(bootstrap_classpath, argv[i]);
            } else if (strcmp(name, "-cp") == 0) { // parse Class Path
                if (++i >= argc) {
                    jvm_abort("缺少参数：%s\n", name);
                }
                strcpy(user_classpath, argv[i]);
            } else {
                jvm_abort("unknown 参数: %s\n", name);
            }
        } else {
            strcpy(main_class_name, argv[i]);
        }
    }

    // 如果 main_class_name 有 .class 后缀，去掉后缀。
    char *p = strrchr(main_class_name, '.');
    if (p != nullptr && strcmp(p, ".class") == 0) {
        *p = 0;
    }

    // parse bootstrap classpath
    if (bootstrap_classpath[0] == 0) { // empty
        // 命令行参数没有设置 bootstrap_classpath 的值，那么使用 JAVA_HOME 环境变量
        char *javaHome = getenv("JAVA_HOME"); // JAVA_HOME 是 JDK 的目录
        if (javaHome == nullptr) {
            raiseException(INTERNAL_ERROR, "no java lib"); // todo
        }
        strcpy(bootstrap_classpath, javaHome);
        strcat(bootstrap_classpath, "/jre/lib");
    }

    findJars(bootstrap_classpath, jreLibJars);

    // 第0个位置放rt.jar，因为rt.jar常用，所以放第0个位置首先搜索。
    for (auto iter = jreLibJars.begin(); iter != jreLibJars.end(); iter++) {
        auto i = iter->rfind('\\');
        auto j = iter->rfind('/');
        if ((i != iter->npos && iter->compare(i + 1, 6, "rt.jar") == 0)
            || (j != iter->npos && iter->compare(j + 1, 6, "rt.jar") == 0)) {
            std::swap(*(jreLibJars.begin()), *iter);
            break;
        }
    }

    // parse extension classpath
    if (extension_classpath[0] == 0) {  // empty
        strcpy(extension_classpath, bootstrap_classpath);
        strcat(extension_classpath, "/ext");  // todo JDK9+ 的目录结构有变动！！！！！！！
    }

    findJars(extension_classpath, jreExtJars);

    // parse user classpath
    if (user_classpath[0] == 0) {  // empty
        char *classpath = getenv("CLASSPATH");
        if (classpath == nullptr) {
            char buf[PATH_MAX + 1];
            getcwd(buf, PATH_MAX); // current working path
            userDirs.emplace_back(buf);
        } else {
            userDirs.emplace_back(classpath);
        }
    } else {
        const char *delim = ";"; // 各个path以分号分隔
        char *path = strtok(user_classpath, delim);
        while (path != nullptr) {
            const char *suffix = strrchr(path, '.');
            if (suffix != nullptr && strcmp(suffix, ".jar") == 0) { // jar file
                userJars.emplace_back(path);
            } else { // directory
                userDirs.emplace_back(path);
            }
            path = strtok(nullptr, delim);
        }
    }

//    time_t time2;
//    time(&time2);
//
//    printf("find jars: %lds\n", ((long)(time2)) - ((long)(time1)));

    register_all_native_methods(); // todo 不要一次全注册，需要时再注册

    g_str_pool = new StrPool;
    init_symbol();
    initBootClassLoader();

//    time_t time3;
//    time(&time3);
//
//    printf("init...: %lds\n", ((long)(time3)) - ((long)(time2)));

    initMainThread();

//    time_t time4;
//    time(&time4);
//
//    printf("initMainThread: %lds\n", ((long)(time4)) - ((long)(time3)));

    TRACE("init main thread over\n");
    // 先加载 sun.mis.VM 类，然后执行其类初始化方法
    Class *vm_class = loadSysClass("sun/misc/VM");
    if (vm_class == nullptr) {
        jvm_abort("vm_class is null\n");  // todo throw exception
        return;
    }
    // VM类的 "initialize~()V" 方法需调用执行
    // 在VM类的类初始化方法中调用了 "initialize" 方法。
    vm_class->clinit();

//    time_t time5;
//    time(&time5);
//
//    printf("sun/misc/VM clinit: %lds\n", ((long)(time5)) - ((long)(time4)));
}

/*
 * The entry of this JVM
 */
int runJVM(int argc, char* argv[])
{
    time_t time1;
    time(&time1);

    initJVM(argc, argv);
    TRACE("init jvm is over.\n");

    time_t time2;
    time(&time2);

    if (main_class_name[0] == 0) {  // empty
        jvm_abort("no input file\n");
    }

    Class *main_class = bootClassLoader->loadClass(main_class_name);
    Method *main_method = main_class->lookupStaticMethod(S(main), S(_array_java_lang_String__V));
    if (main_method == nullptr) {
        jvm_abort("can't find method main."); // todo
    } else {
        if (!main_method->isPublic()) {
            jvm_abort("method main must be public."); // todo
        }
        if (!main_method->isStatic()) {
            jvm_abort("method main must be static."); // todo
        }
    }

    createVMThread(gcLoop); // gc thread

    // 开始在主线程中执行 main 方法
    TRACE("begin to execute main function.\n");
    execJavaFunc(main_method, (Object *) nullptr); //  todo

    // todo 如果有其他的非后台线程在执行，则main线程需要在此wait

    // todo main_thread 退出，做一些清理工作。

    time_t time3;
    time(&time3);

//    printf("init jvm: %lds\n", ((long)(time2)) - ((long)(time1)));
    printf("run jvm: %lds\n", ((long)(time3)) - ((long)(time1)));
    return 0;
}
