#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>



#include "stack_overflow_demo.h"

void infinite_recursion() {
    char a[1024];
     infinite_recursion();
}


#ifdef _WIN32


#include <windows.h>  

// Filter for the stack overflow exception.  
// This function traps the stack overflow exception, but passes  
// all other exceptions through.   
int stack_overflow_exception_filter(int exception_code)
{
    if (exception_code == EXCEPTION_STACK_OVERFLOW)
    {
        // Do not call _resetstkoflw here, because  
        // at this point, the stack is not yet unwound.  
        // Instead, signal that the handler (the __except block)  
        // is to be executed.  
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

int stack_overflow_demo()
{
    int result = 0;

    __try
    {
        infinite_recursion();

    }

    __except (stack_overflow_exception_filter(GetExceptionCode()))
    {
        // Here, it is safe to reset the stack.  

           
    printf("stack overflow\n");
    result = _resetstkoflw();
         
    }

    // Terminate if _resetstkoflw failed (returned 0)  
    if (!result) {
        return 3;
    }
    else {
        printf("I'm back\n");
    }
            
    return 0;
}

jmp_buf buff;

LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{

    printf( "VectoredExceptionHandler %x\n",  pExceptionInfo->ExceptionRecord->ExceptionCode);

    longjmp(buff, 1);

    return EXCEPTION_CONTINUE_SEARCH;
}

int stack_overflow_demo_win_with_hanlder()
{
    PVOID handle = AddVectoredExceptionHandler(1, VectoredExceptionHandler);


    ULONG stackSize = 1024 * 5;
    SetThreadStackGuarantee(&stackSize);
    
    //foo();
    if (setjmp(buff) == 0) {
        infinite_recursion();
    }
    else {
        printf("I'm back\n");
    }

    _resetstkoflw();
    RemoveVectoredExceptionHandler(handle);
    return 0;
}

#else

#define __USE_GNU 1
#define _GNU_SOURCE //for pthread_getattr_np
#include <signal.h>

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <unistd.h>
#include <ucontext.h>

#include <pthread.h>

void* stackTop;


sigjmp_buf b1;


static char stack[SIGSTKSZ];

static int downwardsGrowingStacksChecker(uintptr_t currentSP, uintptr_t stackLimit){
    return currentSP <= stackLimit;
}

static int upwardsGrowingStacksChecker(uintptr_t currentSP, uintptr_t stackLimit){
    return currentSP >= stackLimit;
}



static void handler(int signalId, siginfo_t *si, void *ptr)
{
    if(signalId == SIGSEGV){

        void* faultAddress = si->si_addr;

        ucontext_t *uc = (ucontext_t *)ptr;

        #ifdef __APPLE__
        uintptr_t* sp = (uintptr_t*)uc->uc_stack.ss_sp;
        uintptr_t* stackAddr = (uintptr_t*)&stack;

        if(sp >= stackAddr && sp >= stackAddr + SIGSTKSZ) {
            printf("stack overflow\n");
            siglongjmp(b1, 1);
        } else {
            printf("Segment Fault\n");
            exit(1);
        }

        #else

        uintptr_t safeStackAddress = (uintptr_t)stackTop + sizeof(size_t);
    
        #if (defined __x86_64__ || defined _M_X64 )
        void* stackRegister = (void*)uc->uc_mcontext.gregs[REG_RSP];
        #define STACK_CHECKER downwardsGrowingStacksChecker
        #elif defined __aarch64__ || defined _M_ARM
        void* stackRegister = (void*)uc->uc_mcontext.sp;
        #define STACK_CHECKER downwardsGrowingStacksChecker
        #else
        #error "unsupported CPU"
        #endif

        
        
        if(STACK_CHECKER((uintptr_t)stackRegister,safeStackAddress)){
            printf("stack overflow\n");
            siglongjmp(b1, 1);
        }else{
            printf("Segment Fault\n");

            exit(1);
        }
        #endif
    }else{
        printf("Unhandled Fault %d\n", signalId);

        exit(1);
    }
}

static void normal_sigment_fault_demo(){
    
    struct sigaction sa = {0};
    sa.sa_sigaction  = handler;
    sa.sa_flags = SA_SIGINFO;
    sigfillset(&sa.sa_mask);
    int error = sigaction(SIGSEGV, &sa, 0);

    if(error){
        perror("sigaction error");
    }else{
        int* pm = 0;
        int m = *pm;
    }

    
}


int stack_overflow_demo(){
    stack_t ss; 
    ss.ss_size = SIGSTKSZ;
    ss.ss_sp = stack;
    ss.ss_flags = 0;
    
    struct sigaction sa;
    sa.sa_sigaction  = handler;
    sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
    
    sigaltstack(&ss, 0);
    sigfillset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, 0);

    #ifdef __linux__
    pthread_t thread = pthread_self();
    pthread_attr_t attrs;
    pthread_getattr_np(thread, &attrs);
    size_t stack_size;
    pthread_attr_getstack(&attrs, &stackTop, &stack_size);
    #endif

    if(!sigsetjmp(b1, 1)){
        infinite_recursion();
    }else{
        printf("I'm back\n");
    }

    normal_sigment_fault_demo();

    return 0;
}

#endif