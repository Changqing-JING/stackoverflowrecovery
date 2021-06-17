#include <stdio.h>
#include <stdlib.h>

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

#else



#include <signal.h>
#include <setjmp.h>
#include <unistd.h>


jmp_buf b1;






static char stack[SIGSTKSZ];

void handler(int sig)
{
    printf("stack overflow\n");
    longjmp(b1, 1);
}


int stack_overflow_demo(){
    stack_t ss; 
    ss.ss_size = SIGSTKSZ;
    ss.ss_sp = stack;
    ss.ss_flags = 0;
    
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = SA_ONSTACK;
    
    sigaltstack(&ss, 0);
    sigfillset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, 0);


    if(!setjmp(b1)){
        infinite_recursion();
    }else{
        printf("I'm back\n");
    }

    return 0;
}

#endif