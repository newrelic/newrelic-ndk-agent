#include <jni.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <pthread.h>
#include "procfs.h"

const char *get_arch() {
#if defined(__arm__)
    return "ARM32";
#elif defined(__aarch64__)
    return "ARM64";
#elif defined(__i386__)
    return "X86";
#elif defined(__x86_64__)
    return "X86_64";
#else
#error Unknown architecture!
#endif
}


void crashBySignal(int signo) {
    switch (signo) {
        case SIGSEGV: {
            char **ptr = (char **) 0;
            *ptr = (char *) "Boomshalocklock";
            break;
        }
        case SIGFPE: {
            int denom = 0;
            int dbz = 13 / denom;
            dbz;
            break;
        }
        case SIGABRT:
        case SIGTRAP:
        case SIGBUS:
        case SIGILL:
            syscall(SYS_tgkill, getpid(), gettid(), signo);
            waitpid(getpid(), nullptr, 0);
            break;

        case SIGQUIT:   // ANR
            syscall(SYS_tgkill, getpid(), gettid(), SIGQUIT);
            break;
    }
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_newrelic_agent_android_ndk_MainActivity_installNative(JNIEnv *env, jobject thiz) {
    char str[0x200];
    std::string cstr;
    const char *procName = procfs::get_process_name(getpid(), cstr);

    snprintf(str, sizeof(str),
             "Process[%s]\n"
             "ABI[%s] \n"
             "pid[%d]\n"
             "ppid[%d]\n"
             "thread[%d]\n",
             procName, get_arch(), getpid(), getppid(), gettid());

    return env->NewStringUTF(str);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_MainActivity_raiseSignal(__unused JNIEnv *env, __unused jobject thiz, jint signo) {
    crashBySignal(signo);
    return nullptr;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_MainActivity_raiseException(JNIEnv *env, jobject thiz, jint exception_name_id) {
    using namespace std;

    switch (exception_name_id) {
        case 0:
            throw logic_error("");
            break;
        case 1:
            throw domain_error("");
            break;
        case 2:
            throw invalid_argument("");
            break;
        case 3:
            throw length_error("");
            break;
        case 4:
            throw out_of_range("");
            break;
        case 5:
            throw runtime_error("");
        case 6:
            throw range_error("");
        case 7:
            throw overflow_error("");
        case 8:
            throw underflow_error("");
            break;
        case 9:
            throw bad_cast();
            break;
        case 10:
            throw bad_alloc();
            break;
    };  // switch
}


typedef struct _thread_args {
    int signo;
    size_t sleepSec;
    size_t thread_count;
    bool detach;
    bool join;
} _thread_args_t;

void *crashing_thread(void *args) {
    _thread_args_t *thread_args = (_thread_args_t *) args;
    char buffer[16];

    std::snprintf(buffer, sizeof(buffer) - 1, "Worker-%d-%d", gettid(), thread_args->signo);
    _LOGD("Detached thread running on thread [%s]", buffer);
    pthread_setname_np(pthread_self(), buffer);

    sleep(thread_args->sleepSec);
    crashBySignal(thread_args->signo);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_newrelic_agent_android_ndk_MainActivity_backgroundCrash(JNIEnv *env, jobject thiz,
                             jint signo, jint sleep_sec, jint thread_cnt, jboolean detached) {

    _thread_args_t *thread_args = new _thread_args_t();

    thread_args->signo = signo;
    thread_args->thread_count = thread_cnt;
    thread_args->sleepSec = sleep_sec;
    thread_args->detach = detached;

    pthread_t threadInfo = {};
    pthread_attr_t threadAttr;

    pthread_attr_init(&threadAttr);

    if (detached) {
        pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    }

    for (int i = 0; i < thread_cnt; i++) {
        if (pthread_create(&threadInfo, &threadAttr, crashing_thread, thread_args) != 0) {
            _LOGE("threaded_delegate_call: thread creation failed");
            // release the memory alloc'd above
            pthread_attr_destroy(&threadAttr);
            free(thread_args);
            return nullptr;
        }
    }

    pthread_attr_destroy(&threadAttr);

    return nullptr;
}