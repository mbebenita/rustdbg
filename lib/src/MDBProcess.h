#ifndef MDBPROCESS_H
#define MDBPROCESS_H

struct MDBDyldAllImageInfos32 {
    uint32_t                        version;        /* 1 in Mac OS X 10.4 and 10.5 */
    uint32_t                        infoArrayCount;
    uint32_t                        infoArray;
    uint32_t                        notification;
    bool                            processDetachedFromSharedRegion;
    /* the following fields are only in version 2 (Mac OS X 10.6, iPhoneOS 2.0) and later */
    bool                            libSystemInitialized;
    uint32_t                        dyldImageLoadAddress;
    /* the following field is only in version 3 (Mac OS X 10.6) and later */
    uint32_t                        jitInfo;
    /* the following fields are only in version 5 (Mac OS X 10.6) and later */
    uint32_t                        dyldVersion;
    uint32_t                        errorMessage;
    uint32_t                        terminationFlags;
    /* the following field is only in version 6 (Mac OS X 10.6) and later */
    uint32_t                        coreSymbolicationShmPage;
    /* the following field is only in version 7 (Mac OS X 10.6) and later */
    uintptr_t                       systemOrderFlag;
};

struct MDBDyldImageInfo32 {
    uint32_t                    imageLoadAddress;   /* base address image is mapped into */
    uint32_t                    imageFilePath;      /* path dyld used to load the image */
    uint32_t                    imageFileModDate;   /* time_t of image file */
                                                    /* if stat().st_mtime of imageFilePath does not match imageFileModDate, */
                                                    /* then file has been modified since dyld loaded it */
};

/* A structure filled in by dyld in the inferior process.
   Each dylib/bundle loaded has one of these structures allocated
   for it.
   Each field is either 4 or 8 bytes, depending on the wordsize of
   the inferior process.  (including the modtime field - size_t goes to
   64 bits in the 64 bit ABIs).  */

struct MDBDyldRawInfo {
    uint32_t address;               /* struct mach_header *imageLoadAddress */
    uint32_t name;                  /* const char *imageFilePath */
    uint64_t modtime;               /* time_t imageFileModDate */
};

class MDBProcess {
private:
    mach_vm_address_t findAddress(uint32_t fileType, const char *imageName);
    MDBBreakpoint *onLibraryLoadedBreakpoint;
    void onLibraryLoaded(MDBBreakpoint *breakpoint);

    mach_port_t profiling_port;

public:
    bool running;
    bool initializeDylinkerHooks();
    MDBDebugger *debugger;
    mach_port_t task;
    pid_t pid;
    bool is64Bit;
    MBList<MDBThread *> threads;
    MDBProcess(MDBDebugger *debugger, mach_port_t task, pid_t pid);
    virtual ~MDBProcess();

    void logMemoryState();
    void logDylinkerState();
    void logExecutionState(const char *name);


    bool read_overwrite(void *dst, vm_address_t src, vm_size_t size);
    bool read(mach_vm_address_t *dst, mach_vm_address_t src, mach_vm_size_t size);
    bool free(mach_vm_address_t src, mach_vm_size_t size);

    MDBThread *findThread(thread_t thread);
    bool gatherThreads();
    bool machUpdateThread(MDBThread *thread);
    bool machCommitThread(MDBThread *thread);

    bool machResumeThreadsAndTask();
    bool machResumeThread(MDBThread *thread);
    bool machUpdateTask();
    bool machResumeTask();
    bool machSuspendThread(MDBThread *thread);
    bool machSuspendAllThreads();

    void machSignalTerminated();

    bool machIs64Bit();

    void machSample();
};

#endif /* MDBPROCESS_H */
