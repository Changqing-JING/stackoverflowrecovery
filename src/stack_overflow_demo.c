#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>



#include "stack_overflow_demo.h"

void infinite_recursion() {
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
    AddVectoredExceptionHandler(1, VectoredExceptionHandler);


    ULONG stackSize = 1024 * 5;
    SetThreadStackGuarantee(&stackSize);
    
    //foo();
    if (setjmp(buff) == 0) {
        infinite_recursion();
    }
    else {
        printf("I'm back\n");
    }


    return 0;
}

#else

#define __USE_GNU 1
#define _GNU_SOURCE //for pthread_getattr_np
#include <signal.h>

#include <unistd.h>
#include <ucontext.h>

#include <pthread.h>

void* stackTop;


sigjmp_buf b1;


static char stack[SIGSTKSZ];

static void handler(int signalId, siginfo_t *si, void *ptr)
{
    ucontext_t *uc = (ucontext_t *)ptr;
    void* rp2 = si->si_addr;
    void* rsp = (void*)uc->uc_mcontext.gregs[REG_RSP];
    if(rsp == stackTop){
        printf("stack overflow\n");
        siglongjmp(b1, 1);
    }else{
        printf("Other Fault\n");

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

    pthread_t thread = pthread_self();
    pthread_attr_t attrs;
    pthread_getattr_np(thread, &attrs);
    size_t stack_size;
    pthread_attr_getstack(&attrs, &stackTop, &stack_size);

    if(!sigsetjmp(b1, 1)){
        infinite_recursion();
    }else{
        printf("I'm back\n");
    }

    normal_sigment_fault_demo();

    return 0;
}

#endif