刚开始入门学习honggfuzz，先记录一下fuzz的流程，我们先从honggfuzz.c的main函数看起：
首先在最开始的解析命令行参数的cmdlineParse函数中，会将hfuzz.feedback.dynFileMethod默认设置为_HF_DYNFILE_SOFT，即基于软件的反馈驱动fuzz。如果命令行中有-x选项，表示采用static/dry mode，即不采用反馈驱动。

```
bool cmdlineParse(int argc, char* argv[], honggfuzz_t* hfuzz) {
    *hfuzz = (honggfuzz_t){
        
        ..........
        
        .feedback =
            {
                .feedbackMap = NULL,
                .feedback_mutex = PTHREAD_MUTEX_INITIALIZER,
                .bbFd = -1,
                .blacklistFile = NULL,
                .blacklist = NULL,
                .blacklistCnt = 0,
                .skipFeedbackOnTimeout = false,
                .dynFileMethod = _HF_DYNFILE_SOFT,
                .state = _HF_STATE_UNSET,
            },
        
        ........
            
    };

```

跳过上面的初始化流程，我们来到：
```
    setupRLimits();
    setupSignalsPreThreads();
    fuzz_threadsStart(&hfuzz);
```
fuzz_threadsStart的参数是一个结构体，下面是他的定义：
```
typedef struct {
    struct {
        size_t threadsMax;
        size_t threadsFinished;
        uint32_t threadsActiveCnt;
        pthread_t mainThread;
        pid_t mainPid;
        pthread_t threads[_HF_THREAD_MAX];
    } threads;
    struct {
        const char* inputDir;
        const char* outputDir;
        DIR* inputDirPtr;
        size_t fileCnt;
        const char* fileExtn;
        bool fileCntDone;
        size_t newUnitsAdded;
        char workDir[PATH_MAX];
        const char* crashDir;
        const char* covDirNew;
        bool saveUnique;
        size_t dynfileqCnt;
        pthread_rwlock_t dynfileq_mutex;
        struct dynfile_t* dynfileqCurrent;
        TAILQ_HEAD(dyns_t, dynfile_t) dynfileq;
        bool exportFeedback;
    } io;
    struct {
        int argc;
        const char* const* cmdline;
        bool nullifyStdio;
        bool fuzzStdin;
        const char* externalCommand;
        const char* postExternalCommand;
        const char* feedbackMutateCommand;
        bool netDriver;
        bool persistent;
        uint64_t asLimit;
        uint64_t rssLimit;
        uint64_t dataLimit;
        uint64_t coreLimit;
        bool clearEnv;
        char* env_ptrs[128];
        char env_vals[128][4096];
        sigset_t waitSigSet;
    } exe;
    struct {
        time_t timeStart;
        time_t runEndTime;
        time_t tmOut;
        time_t lastCovUpdate;
        int64_t timeOfLongestUnitInMilliseconds;
        bool tmoutVTALRM;
    } timing;
    struct {
        const char* dictionaryFile;
        TAILQ_HEAD(strq_t, strings_t) dictq;
        size_t dictionaryCnt;
        size_t mutationsMax;
        unsigned mutationsPerRun;
        size_t maxFileSz;
    } mutate;
    struct {
        bool useScreen;
        char cmdline_txt[65];
        int64_t lastDisplayMillis;
    } display;
    struct {
        bool useVerifier;
        bool exitUponCrash;
        const char* reportFile;
        pthread_mutex_t report_mutex;
        size_t dynFileIterExpire;
        bool only_printable;
        bool minimize;
        bool switchingToFDM;
    } cfg;
    struct {
        bool enable;
        bool del_report;
    } sanitizer;
    struct {
        fuzzState_t state;
        feedback_t* feedbackMap;
        int bbFd;
        pthread_mutex_t feedback_mutex;
        const char* blacklistFile;
        uint64_t* blacklist;
        size_t blacklistCnt;
        bool skipFeedbackOnTimeout;
        dynFileMethod_t dynFileMethod;
    } feedback;
    struct {
        size_t mutationsCnt;
        size_t crashesCnt;
        size_t uniqueCrashesCnt;
        size_t verifiedCrashesCnt;
        size_t blCrashesCnt;
        size_t timeoutedCnt;
    } cnts;
    struct {
        bool enabled;
        int serverSocket;
        int clientSocket;
    } socketFuzzer;
    /* For the Linux code */
    struct {
        int exeFd;
        hwcnt_t hwCnts;
        uint64_t dynamicCutOffAddr;
        bool disableRandomization;
        void* ignoreAddr;
        const char* symsBlFile;
        char** symsBl;
        size_t symsBlCnt;
        const char* symsWlFile;
        char** symsWl;
        size_t symsWlCnt;
        uintptr_t cloneFlags;
        bool kernelOnly;
        bool useClone;
    } linux;
    /* For the NetBSD code */
    struct {
        void* ignoreAddr;
        const char* symsBlFile;
        char** symsBl;
        size_t symsBlCnt;
        const char* symsWlFile;
        char** symsWl;
        size_t symsWlCnt;
    } netbsd;
} honggfuzz_t;
```

接下来进入fuzz_threadsStart：

```
void fuzz_threadsStart(honggfuzz_t* hfuzz) {
    if (!arch_archInit(hfuzz)) {
        LOG_F("Couldn't prepare arch for fuzzing");
    }
    if (!sanitizers_Init(hfuzz)) {
        LOG_F("Couldn't prepare sanitizer options");
    }

    if (hfuzz->socketFuzzer.enabled) {
        /* Don't do dry run with socketFuzzer */
        LOG_I("Entering phase - Feedback Driven Mode (SocketFuzzer)");
        hfuzz->feedback.state = _HF_STATE_DYNAMIC_MAIN;
    } else if (hfuzz->feedback.dynFileMethod != _HF_DYNFILE_NONE) {
        LOG_I("Entering phase 1/3: Dry Run");
        hfuzz->feedback.state = _HF_STATE_DYNAMIC_DRY_RUN;
    } else {
        LOG_I("Entering phase: Static");
        hfuzz->feedback.state = _HF_STATE_STATIC;
    }

    for (size_t i = 0; i < hfuzz->threads.threadsMax; i++) {
        if (!subproc_runThread(
                hfuzz, &hfuzz->threads.threads[i], fuzz_threadNew, /* joinable= */ true)) {
            PLOG_F("Couldn't run a thread #%zu", i);
        }
    }
}
```
fuzz_threadsStart函数中不是static/dry mode设置当前state为_HF_STATE_DYNAMIC_DRY_RUN，进入第一阶段Dry Run。

接下来调用subproc_runThread
```
bool subproc_runThread(
    honggfuzz_t* hfuzz, pthread_t* thread, void* (*thread_func)(void*), bool joinable) {
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(
        &attr, joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, _HF_PTHREAD_STACKSIZE);
    pthread_attr_setguardsize(&attr, (size_t)sysconf(_SC_PAGESIZE));

    if (pthread_create(thread, &attr, thread_func, (void*)hfuzz) < 0) {
        PLOG_W("Couldn't create a new thread");
        return false;
    }

    pthread_attr_destroy(&attr);

    return true;
}
```
在该函数中通过pthread_create函数来调用fuzz_threadNew函数。
fuzz_threadNew调用了fuzz_fuzzLoop函数：
```
    ......
    
        if (hfuzz->socketFuzzer.enabled) {
            fuzz_fuzzLoopSocket(&run);
        } else {
            fuzz_fuzzLoop(&run);
        }
    
    ......
    
```
cfuzz_fuzzLoop函数主要需要注意fuzz_fetchInput和subproc_Run这两个函数。
```
static voidcfuzz_fuzzLoop(run_t* run) {
    
    ...........
    
    if (!fuzz_fetchInput(run)) {
        if (run->global->cfg.minimize && fuzz_getState(run->global) == _HF_STATE_DYNAMIC_MINIMIZE) {
            fuzz_setTerminating();
            LOG_I("Corpus minimization done!");
            return;
        }
        LOG_F("Cound't prepare input for fuzzing");
    }
    if (!subproc_Run(run)) {
        LOG_F("Couldn't run fuzzed command");
    }

    if (run->global->feedback.dynFileMethod != _HF_DYNFILE_NONE) {
        fuzz_perfFeedback(run);
    }
    if (run->global->cfg.useVerifier && !fuzz_runVerifier(run)) {
        return;
    }
    report_saveReport(run);
}
```

fuzz_fetchInput:
```
static bool fuzz_fetchInput(run_t* run) {
    {
        fuzzState_t st = fuzz_getState(run->global);
        if (st == _HF_STATE_DYNAMIC_DRY_RUN) {
            run->mutationsPerRun = 0U;
            if (input_prepareStaticFile(run, /* rewind= */ false, true)) {
                return true;
            }
            fuzz_setDynamicMainState(run);
            run->mutationsPerRun = run->global->mutate.mutationsPerRun;
        }
    }
```
因为当前的state是_HF_STATE_DYNAMIC_DRY_RUN，所以接着调用了input_prepareStaticFile函数取得一个文件并返回。

subproc_Run:

```
bool subproc_Run(run_t* run) {
    run->timeStartedMillis = util_timeNowMillis();
    
    if (!subproc_New(run)) {
        LOG_E("subproc_New()");
        return false;
    }

    arch_prepareParent(run);
    arch_reapChild(run);

    int64_t diffMillis = util_timeNowMillis() - run->timeStartedMillis;
    if (diffMillis >= run->global->timing.timeOfLongestUnitInMilliseconds) {
        run->global->timing.timeOfLongestUnitInMilliseconds = diffMillis;
    }

    return true;
}
```
subproc_Run函数首先调用了subproc_New函数，在subproc_New函数中clone出一个子进程调用arch_launchChild函数，在arch_launchChild函数中运行了被fuzz的程序。
```
static bool subproc_New(run_t* run) {
    if (run->pid) {
        return true;
    }
    
    int sv[2];
    if (run->global->exe.persistent) {
        if (run->persistentSock != -1) {
            close(run->persistentSock);
        }

        int sock_type = SOCK_STREAM;
#if defined(SOCK_CLOEXEC)
        sock_type |= SOCK_CLOEXEC;
#endif
        if (socketpair(AF_UNIX, sock_type, 0, sv) == -1) {
            PLOG_W("socketpair(AF_UNIX, SOCK_STREAM, 0, sv)");
            return false;
        }
        run->persistentSock = sv[0];
    }

    LOG_D("Forking new process for thread: %" PRId32, run->fuzzNo);

    run->pid = arch_fork(run);
    if (run->pid == -1) {
        PLOG_E("Couldn't fork");
        run->pid = 0;
        return false;
    }
    /* The child process */
    if (!run->pid) {
        logMutexReset();
        alarm(1);
        signal(SIGALRM, SIG_DFL);

        if (run->global->exe.persistent) {
            if (TEMP_FAILURE_RETRY(dup2(sv[1], _HF_PERSISTENT_FD)) == -1) {
                PLOG_F("dup2('%d', '%d')", sv[1], _HF_PERSISTENT_FD);
            }
            close(sv[0]);
            close(sv[1]);
        }

        if (!subproc_PrepareExecv(run)) {
            LOG_E("subproc_PrepareExecv() failed");
            exit(EXIT_FAILURE);
        }
        if (!arch_launchChild(run)) {
            LOG_E("Error launching child process");
            kill(run->global->threads.mainPid, SIGTERM);
            _exit(1);
        }
        abort();
    }

    /* Parent */
    LOG_D("Launched new process, pid=%d, thread: %" PRId32 " (concurrency: %zd)", (int)run->pid,
        run->fuzzNo, run->global->threads.threadsMax);

    arch_prepareParentAfterFork(run);

    if (run->global->exe.persistent) {
        close(sv[1]);
        run->runState = _HF_RS_WAITING_FOR_INITIAL_READY;
        LOG_I("Persistent mode: Launched new persistent pid=%d", (int)run->pid);
    }

    return true;
}
```

subproc_New函数返回后调用arch_reapChild函数，arch_reapChild函数中调用了arch_checkWait函数。
```
static bool arch_checkWait(run_t* run) {
    /* All queued wait events must be tested when SIGCHLD was delivered */
    for (;;) {
        int status;
        /* Wait for the whole process group of run->pid */
        pid_t pid = TEMP_FAILURE_RETRY(wait6(P_SID, run->pid, &status,
            WALLSIG | WALTSIG | WTRAPPED | WEXITED | WUNTRACED | WCONTINUED | WSTOPPED | WNOHANG,
            NULL, NULL));
        if (pid == 0) {
            return false;
        }
        if (pid == -1 && errno == ECHILD) {
            LOG_D("No more processes to track");
            return true;
        }
        if (pid == -1) {
            PLOG_F("wait6(pid/session=%d) failed", (int)run->pid);
        }

        arch_traceAnalyze(run, status, pid);

        char statusStr[4096];
        LOG_D("pid=%d returned with status: %s", pid,
            subproc_StatusToStr(status, statusStr, sizeof(statusStr)));

        if (pid == run->pid && (WIFEXITED(status) || WIFSIGNALED(status))) {
            if (run->global->exe.persistent) {
                if (!fuzz_isTerminating()) {
                    LOG_W("Persistent mode: PID %d exited with status: %s", pid,
                        subproc_StatusToStr(status, statusStr, sizeof(statusStr)));
                }
            }
            return true;
        }
    }
}

-----------------------------------------------------------

void arch_traceAnalyze(run_t* run, int status, pid_t pid) {
    
    ........
    
    if (WIFSTOPPED(status)) {
        /*
         * If it's an interesting signal, save the testcase
         */
        if (arch_sigs[WSTOPSIG(status)].important) {
            /*
             * If fuzzer worker is from core fuzzing process run full
             * analysis. Otherwise just unwind and get stack hash signature.
             */
            if (run->mainWorker) {
                arch_traceSaveData(run, pid);
            } else {
                arch_traceAnalyzeData(run, pid);
            }
        }
       
       .......
       
}
```
arch_checkWait函数等待子进程返回并调用arch_traceAnalyze函数。如果子进程返回状态为暂停，并且是我们感兴趣的信号时，如果是fuzz进程则调用arch_traceSaveData函数(fuzz_fuzzLoop函数调用subproc_Run函数的情况，下文同)；如果是其它进程则调用arch_traceAnalyzeData函数(fuzz_fuzzLoop函数调用fuzz_runVerifier函数的情况，下文同)。前者进行的是完整的分析，后者仅仅栈回溯然后计算stack hash。
```
这部分代码，源码的注释已经很详细了，就不重点分析了
```
接下来返回到fuzz_fuzzLoop函数，最后调用fuzz_perfFeedback函数更新代码覆盖率相关信息，fuzz_runVerifier函数指示是否应该使用当前验证的crash更新report。在fuzz_perfFeedback函数中如果当前的文件增加了代码覆盖率调用input_addDynamicInput函数将它加到语料库中。
```
static void fuzz_perfFeedback(run_t* run) {

    ...................

        /* Any increase in coverage (edge, pc, cmp, hw) counters forces adding input to the corpus */
    if (run->linux.hwCnts.newBBCnt > 0 || softCntPc > 0 || softCntEdge > 0 || softCntCmp > 0 ||
        diff0 < 0 || diff1 < 0) {
        if (diff0 < 0) {
            run->global->linux.hwCnts.cpuInstrCnt = run->linux.hwCnts.cpuInstrCnt;
        }
        if (diff1 < 0) {
            run->global->linux.hwCnts.cpuBranchCnt = run->linux.hwCnts.cpuBranchCnt;
        }
        run->global->linux.hwCnts.bbCnt += run->linux.hwCnts.newBBCnt;
        run->global->linux.hwCnts.softCntPc += softCntPc;
        run->global->linux.hwCnts.softCntEdge += softCntEdge;
        run->global->linux.hwCnts.softCntCmp += softCntCmp;

        if (run->global->cfg.minimize) {
            LOG_I("Keeping '%s' in '%s'", run->origFileName,
                run->global->io.outputDir ? run->global->io.outputDir : run->global->io.inputDir);
            if (run->global->io.outputDir && !input_writeCovFile(run->global->io.outputDir,
                                                 run->dynamicFile, run->dynamicFileSz)) {
                LOG_E("Couldn't save the coverage data to '%s'", run->global->io.outputDir);
            }
        } else {
            LOG_I("Size:%zu (i,b,hw,ed,ip,cmp): %" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64
                  "/%" PRIu64 "/%" PRIu64 ", Tot:%" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64
                  "/%" PRIu64 "/%" PRIu64,
                run->dynamicFileSz, run->linux.hwCnts.cpuInstrCnt, run->linux.hwCnts.cpuBranchCnt,
                run->linux.hwCnts.newBBCnt, softCntEdge, softCntPc, softCntCmp,
                run->global->linux.hwCnts.cpuInstrCnt, run->global->linux.hwCnts.cpuBranchCnt,
                run->global->linux.hwCnts.bbCnt, run->global->linux.hwCnts.softCntEdge,
                run->global->linux.hwCnts.softCntPc, run->global->linux.hwCnts.softCntCmp);

            input_addDynamicInput(run->global, run->dynamicFile, run->dynamicFileSz,
                (uint64_t[4]){0, 0, 0, 0}, "[DYNAMIC]");
        }

        if (run->global->socketFuzzer.enabled) {
            LOG_D("SocketFuzzer: fuzz: new BB (perf)");
            fuzz_notifySocketFuzzerNewCov(run->global);
        }
    } else if (fuzz_getState(run->global) == _HF_STATE_DYNAMIC_MINIMIZE) {
        if (run->global->io.outputDir == NULL) {
            LOG_I("Removing '%s' from '%s'", run->origFileName, run->global->io.inputDir);
            input_removeStaticFile(run->global->io.inputDir, run->origFileName);
        }
    }
}

```
当fuzz_fetchInput调用的input_prepareStaticFile函数无法获取新的文件时，返回false并执行fuzz_setDynamicMainState函数。
```
static bool fuzz_fetchInput(run_t* run) {
    {
        fuzzState_t st = fuzz_getState(run->global);
        if (st == _HF_STATE_DYNAMIC_DRY_RUN) {
            run->mutationsPerRun = 0U;
            if (input_prepareStaticFile(run, /* rewind= */ false, true)) {
                return true;
            }
            fuzz_setDynamicMainState(run);
            run->mutationsPerRun = run->global->mutate.mutationsPerRun;
        }
    }
    
    ...........
    
}
```
在fuzz_setDynamicMainState函数中，设置witchingToFDM为True，进入第二阶段Switching to the Feedback Driven Mode。
```
 LOG_I("Entering phase 2/3: Switching to the Feedback Driven Mode");
 ATOMIC_SET(run->global->cfg.switchingToFDM, true);

    for (;;) {
        /* Check if all threads have already reported in for changing state */
        if (ATOMIC_GET(cnt) == run->global->threads.threadsMax) {
            break;
        }
        if (fuzz_isTerminating()) {
            return;
        }
        util_sleepForMSec(10); /* Check every 10ms */
    }

```
当所有的线程都进入第二阶段以后设置cfg.switchingToFDM为false，之后进行if判断：如果设置了minimize，则进入Corpus Minimization阶段。
```

    if (run->global->cfg.minimize) {
        LOG_I("Entering phase 3/3: Corpus Minimization");
        ATOMIC_SET(run->global->feedback.state, _HF_STATE_DYNAMIC_MINIMIZE);
        return;
    }
```

如果初始模糊没有产生有用的覆盖，只需向动态语料库添加一个空文件，这样动态阶段就不会因为缺少有用的输入而失败
```
    /*
     * If the initial fuzzing yielded no useful coverage, just add a single empty file to the
     * dynamic corpus, so the dynamic phase doesn't fail because of lack of useful inputs
     */
    if (run->global->io.dynfileqCnt == 0) {
        input_addDynamicInput(run->global, (const uint8_t*)"", /* size= */ 0U,
            /* cov */ (uint64_t[4]){0, 0, 0, 0}, /* path= */ "[DYNAMIC]");
    }
    
```
如果没有上述的设置，则会进入Dynamic Main (Feedback Driven Mode)模式。
```    

    snprintf(run->origFileName, sizeof(run->origFileName), "[DYNAMIC]");
    LOG_I("Entering phase 3/3: Dynamic Main (Feedback Driven Mode)");
    ATOMIC_SET(run->global->feedback.state, _HF_STATE_DYNAMIC_MAIN);
}

```
执行返回到fuzz_fetchInput函数，调用input_prepareFileDynamically函数进行变异。
```
        if (!input_prepareDynamicInput(run, false)) {
            LOG_E("input_prepareFileDynamically() failed");
            return false;
        }
```
input_prepareDynamicInput函数进行变异，将之前input_addDynamicInput函数放入语料库的文件进行变异。可以对照下面图片
![](./img/12.png)
