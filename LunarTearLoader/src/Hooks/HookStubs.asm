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
    

    sub rsp, 0x20

    lea rcx, [rsp + 0xF8] ; filename - used previously in target function, stored on stack here       
    ; rdx: stbl data pointer

   
    call HandleSettbllHook


    add rsp, 0x20

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

    sub rsp, 0x30

    lea rcx, [rsp + 0x300] ; filename

    mov [rsp + 0x20], rdx
    mov [rsp + 0x28], r8

    lea rdx, [rsp + 0x20]
    lea r8, [rsp + 0x28]

    call HandleLubHook


    ; The function called immediately after trampolining takes the bytecode buffer in rdx and the buffer size in R8

    mov r8, [rsp + 0x28] ; new bytecode size calculated by the hook
    mov rdx, [rsp + 0x20] ; new bytecode pointer calculated by the hook
    ; both r8 and rdx are intentionally modified and passed into ScriptManager_LoadAndRunBuffer

    add rsp, 0x30

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