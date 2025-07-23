BITS 64

EXTERN HandleSettbllHook

EXTERN PhaseTableTrampoline
EXTERN DroptableTableTrampoline
EXTERN GameTableTrampoline

GLOBAL PhaseTableStub
GLOBAL DroptableTableStub
GLOBAL GameTableStub

SECTION .data
    droptableTableName db "__droptable__.settbll", 0
    gameTableName db "__game__.settbll", 0

SECTION .text

PhaseTableStub:

    pushfq
    push rax
    push rbx
    push rcx
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

    lea rcx, [rbp + 0xD8] ; filename

    call HandleSettbllHook
    mov rsp, rbp

    mov rdx, rax ; move result of hook back into rdx, regardles of wherever it found a loose file

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
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel PhaseTableTrampoline]



DroptableTableStub:

    pushfq
    push rax
    push rbx
    push rcx
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

    lea rcx, [rel droptableTableName] ; filename

    call HandleSettbllHook
    mov rsp, rbp

    mov rdx, rax ; move result of hook back into rdx, regardles of wherever it found a loose file

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
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel DroptableTableTrampoline]


GameTableStub:

    pushfq
    push rax
    push rbx
    push rcx
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

    lea rcx, [rel gameTableName] ; filename

    call HandleSettbllHook
    mov rsp, rbp

    mov rdx, rax ; move result of hook back into rdx, regardles of wherever it found a loose file

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
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel GameTableTrampoline]

