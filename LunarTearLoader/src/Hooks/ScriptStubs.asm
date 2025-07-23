BITS 64

EXTERN HandleLubHook

EXTERN PhaseScriptTrampoline
EXTERN LibScriptTrampoline
EXTERN GameScriptTrampoline
EXTERN RootScriptTrampoline

GLOBAL PhaseScriptStub
GLOBAL LibScriptStub
GLOBAL GameScriptStub
GLOBAL RootScriptStub

SECTION .data

    libScriptName db "__libnier__.lub", 0  
    gameScriptName db "__game__.lub", 0
    rootScriptName db "__root__.lub", 0


SECTION .text

PhaseScriptStub:
    
    pushfq
    push rax
    push rbx
    push rcx
    push rbp
    push rsi
    push rdi
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, rsp
    and rsp, -16
    sub rsp, 0x30

    lea rcx, [rbp + 0x2D0] ; filename
    mov [rsp + 0x20], rdx ; data  pointer
    mov [rsp + 0x28], r8 ; size 

    lea rdx, [rsp + 0x20] ; double pointer to data
    lea r8, [rsp + 0x28] ; size pointer

    call HandleLubHook

    mov rdx, [rsp + 0x20] ; new data pointer 
    mov r8, [rsp + 0x28]  ; new size

    mov rsp, rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop rdi
    pop rsi
    pop rbp
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel PhaseScriptTrampoline]


LibScriptStub:

    pushfq
    push rax
    push rbx
    push rcx
    push rbp
    push rsi
    push rdi
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, rsp
    and rsp, -16
    sub rsp, 0x30

    lea rcx, [rel libScriptName] ; filename
    mov [rsp + 0x20], rdx ; data  pointer
    mov [rsp + 0x28], r8 ; size 

    lea rdx, [rsp + 0x20] ; double pointer to data
    lea r8, [rsp + 0x28] ; size pointer

    call HandleLubHook

    mov rdx, [rsp + 0x20] ; new data pointer 
    mov r8, [rsp + 0x28]  ; new size

    mov rsp, rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop rdi
    pop rsi
    pop rbp
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel LibScriptTrampoline]

   
GameScriptStub:

    pushfq
    push rax
    push rbx
    push rcx
    push rbp
    push rsi
    push rdi
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, rsp
    and rsp, -16
    sub rsp, 0x30

    lea rcx, [rel gameScriptName] ; filename
    mov [rsp + 0x20], rdx ; data pointer
    mov [rsp + 0x28], r8 ; size 

    lea rdx, [rsp + 0x20] ; double pointer to data
    lea r8, [rsp + 0x28] ; size pointer

    call HandleLubHook

    mov rdx, [rsp + 0x20] ; new data pointer 
    mov r8, [rsp + 0x28]  ; new size

    mov rsp, rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop rdi
    pop rsi
    pop rbp
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel GameScriptTrampoline]


RootScriptStub:

    pushfq
    push rax
    push rbx
    push rcx
    push rbp
    push rsi
    push rdi
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, rsp
    and rsp, -16
    sub rsp, 0x30

    lea rcx, [rel rootScriptName] ; filename
    mov [rsp + 0x20], rdx ; data pointer
    mov [rsp + 0x28], r8 ; size 

    lea rdx, [rsp + 0x20] ; double pointer to data
    lea r8, [rsp + 0x28] ; size pointer

    call HandleLubHook

    mov rdx, [rsp + 0x20] ; new data pointer 
    mov r8, [rsp + 0x28]  ; new size

    mov rsp, rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop rdi
    pop rsi
    pop rbp
    pop rcx
    pop rbx
    pop rax
    popfq

    jmp [rel RootScriptTrampoline]