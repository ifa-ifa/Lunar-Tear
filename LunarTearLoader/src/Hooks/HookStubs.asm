BITS 64

EXTERN HandleSettbllHook
EXTERN HandleLubHook
EXTERN pTrampoline_Settbll
EXTERN pTrampoline_Lub

SECTION .text

GLOBAL SettbllAssemblyStub
GLOBAL LubAssemblyStub

SettbllAssemblyStub:

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

    jmp [rel pTrampoline_Settbll]


LubAssemblyStub:
    
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

    ; move args to stack and pass pointers so the handler can modify them
    mov [rsp + 0x20], rdx ; data double pointer
    mov [rsp + 0x28], r8 ; size pointer

    lea rcx, [rbp + 0x2D0] ; filename



    ; Arg2 (RDX): double pointer to data
    lea rdx, [rsp + 0x20]

    ; Arg3 (R8): pointer to size
    lea r8, [rsp + 0x28]

    call HandleLubHook


    ; The function called immediately after trampolining takes the bytecode buffer in rdx and the buffer size in R8

    mov rdx, [rsp + 0x20] ; Load the new data pointer for the original function
    mov r8, [rsp + 0x28]  ; Load the new size for the original function

    ; both r8 and rdx are intentionally modified and passed into ScriptManager_LoadAndRunBuffer

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

    jmp [rel pTrampoline_Lub]