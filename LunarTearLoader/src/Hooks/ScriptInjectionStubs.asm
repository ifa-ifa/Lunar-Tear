BITS 64

GLOBAL PostPhaseStub
GLOBAL PostGameStub
GLOBAL PostRootStub

EXTERN InjectPostPhaseScripts
EXTERN InjectPostRootScripts
EXTERN InjectPostGameScripts

EXTERN PostPhaseTrampoline
EXTERN PostGameTrampoline
EXTERN PostRootTrampoline

; TODO: None of these asm files preserve xmm registers (but none of the injected scripts use them either)

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

PostGameStub:

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

    call InjectPostGameScripts

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

    jmp [rel PostGameTrampoline]

PostRootStub:

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

    call InjectPostRootScripts

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

    jmp [rel PostRootTrampoline]