BITS 64

GLOBAL PostPhaseStub
GLOBAL PostLibStub

EXTERN InjectPostPhaseScripts
EXTERN InjectPostLibScripts
EXTERN PostPhaseTrampoline
EXTERN PostLibTrampoline

SECTION .text

PostPhaseStub:

    pushfq
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, rsp
    and rsp, -16 
    sub rsp, 32

    lea rcx, [rsp+ 608+ 160]

    call InjectPostPhaseScripts

    mov rsp, rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel PostPhaseTrampoline]

PostLibStub:

    pushfq
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, rsp
    and rsp, -16 
    sub rsp, 32

    call InjectPostLibScripts

    mov rsp, rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel PostLibTrampoline]