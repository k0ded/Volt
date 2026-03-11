.code
FiberGetContext PROC
	; rcx = Pointer to context struct
	mov		r8, [rsp]
	mov		[rcx + 8*0], r8	; Store program counter

	lea		r8, [rsp + 8]
	mov		[rcx + 8*1], r8 ; Store stack pointer

    ; Store preserved registers
	mov     [rcx + 8*2], rbx
    mov     [rcx + 8*3], rbp
    mov     [rcx + 8*4], r12
    mov     [rcx + 8*5], r13
    mov     [rcx + 8*6], r14
    mov     [rcx + 8*7], r15
    mov     [rcx + 8*8], rdi
    mov     [rcx + 8*9], rsi

    ; Save XMM registers (16-byte each)
    movups  [rcx + 8*10 + 16*0], xmm6
    movups  [rcx + 8*10 + 16*1], xmm7
    movups  [rcx + 8*10 + 16*2], xmm8
    movups  [rcx + 8*10 + 16*3], xmm9
    movups  [rcx + 8*10 + 16*4], xmm10
    movups  [rcx + 8*10 + 16*5], xmm11
    movups  [rcx + 8*10 + 16*6], xmm12
    movups  [rcx + 8*10 + 16*7], xmm13
    movups  [rcx + 8*10 + 16*8], xmm14
    movups  [rcx + 8*10 + 16*9], xmm15

    ; Save TEB
    mov     r8, gs:[08h]
    mov     [rcx + 8*30], r8
    mov     r8, gs:[10h]
    mov     [rcx + 8*31], r8
    mov     r8, gs:[00h]
    mov     [rcx + 8*32], r8
    mov     r8, gs:[1478h]
    mov     [rcx + 8*33], r8

    ; Save SSE & x87 FPU
    stmxcsr [rcx + 8*34]
    fnstcw  [rcx + 8*34 + 4]

    ; Return
    xor eax, eax
    ret
FiberGetContext ENDP

FiberSetContext PROC
    ; rcx = pointer to context struct
    
    ; Should return to the address set with Get/SwapContext
    mov     r8, [rcx + 8*0]

    ; Load preserved registers
    mov     rbx, [rcx + 8*2]
    mov     rbp, [rcx + 8*3]
    mov     r12, [rcx + 8*4]
    mov     r13, [rcx + 8*5]
    mov     r14, [rcx + 8*6]
    mov     r15, [rcx + 8*7]
    mov     rdi, [rcx + 8*8]
    mov     rsi, [rcx + 8*9]
    movups  xmm6, [rcx + 8*10 + 16*0]
    movups  xmm7, [rcx + 8*10 + 16*1]
    movups  xmm8, [rcx + 8*10 + 16*2]
    movups  xmm9, [rcx + 8*10 + 16*3]
    movups  xmm10, [rcx + 8*10 + 16*4]
    movups  xmm11, [rcx + 8*10 + 16*5]
    movups  xmm12, [rcx + 8*10 + 16*6]
    movups  xmm13, [rcx + 8*10 + 16*7]
    movups  xmm14, [rcx + 8*10 + 16*8]
    movups  xmm15, [rcx + 8*10 + 16*9]

    ; Set TEB
    mov     rax, [rcx + 8*30]
    mov     gs:[08h], rax
    mov     rax, [rcx + 8*31]
    mov     gs:[10h], rax
    mov     rax, [rcx + 8*32]
    mov     gs:[00h], rax
    mov     rax, [rcx + 8*33]
    mov     gs:[1478h], rax

    ; Set SSE & x87 FPU
    ldmxcsr [rcx + 8*34]
    fldcw   [rcx + 8*34 + 4]

    ; Load new stack pointer
    mov     rsp, [rcx + 8*1]

    ; Move user data argument into rcx
    mov     rcx, [rcx + 8*35]

    ; Push RIP to stack for RET.
    push    r8

    ; Return
    xor eax, eax
    ret
FiberSetContext ENDP

FiberSwapContext PROC
	; rcx = Pointer to dst context struct
	mov		rax, [rsp]
	mov		[rcx + 8*0], rax ; Store program counter

	lea		rax, [rsp + 8]
	mov		[rcx + 8*1], rax ; Store stack pointer

    ; Store preserved registers
	mov     [rcx + 8*2], rbx
    mov     [rcx + 8*3], rbp
    mov     [rcx + 8*4], r12
    mov     [rcx + 8*5], r13
    mov     [rcx + 8*6], r14
    mov     [rcx + 8*7], r15
    mov     [rcx + 8*8], rdi
    mov     [rcx + 8*9], rsi

    ; Save XMM registers (16-byte each)
    movups  [rcx + 8*10 + 16*0], xmm6
    movups  [rcx + 8*10 + 16*1], xmm7
    movups  [rcx + 8*10 + 16*2], xmm8
    movups  [rcx + 8*10 + 16*3], xmm9
    movups  [rcx + 8*10 + 16*4], xmm10
    movups  [rcx + 8*10 + 16*5], xmm11
    movups  [rcx + 8*10 + 16*6], xmm12
    movups  [rcx + 8*10 + 16*7], xmm13
    movups  [rcx + 8*10 + 16*8], xmm14
    movups  [rcx + 8*10 + 16*9], xmm15

    ; Save TEB
    mov     rax, gs:[08h]
    mov     [rcx + 8*30], rax
    mov     rax, gs:[10h]
    mov     [rcx + 8*31], rax
    mov     rax, gs:[00h]
    mov     [rcx + 8*32], rax
    mov     rax, gs:[1478h]
    mov     [rcx + 8*33], rax

    ; Save SSE & x87 FPU
    stmxcsr [rcx + 8*34]
    fnstcw  [rcx + 8*34 + 4]

    ; rdx = Pointer to src context struct

    ; Load new stack pointer
    mov     rsp, [rdx + 8*1]

    ; Set TEB
    mov     rax, [rdx + 8*30]
    mov     gs:[08h], rax
    mov     rax, [rdx + 8*31]
    mov     gs:[10h], rax
    mov     rax, [rdx + 8*32]
    mov     gs:[00h], rax
    mov     rax, [rdx + 8*33]
    mov     gs:[1478h], rax

    ; Call SwitchContextCallback
    push    r8
    push    rdx

    mov     rcx, r9 ; Move user data into arg 0
    sub     rsp, 32 ; Make space for the shadow space
    call    r8      ; Call the callback
    add     rsp, 32 ; Clean up shadow space

    pop     rdx
    pop     r8

    ; Load preserved registers
    mov     rbx, [rdx + 8*2]
    mov     rbp, [rdx + 8*3]
    mov     r12, [rdx + 8*4]
    mov     r13, [rdx + 8*5]
    mov     r14, [rdx + 8*6]
    mov     r15, [rdx + 8*7]
    mov     rdi, [rdx + 8*8]
    mov     rsi, [rdx + 8*9]
    movups  xmm6, [rdx + 8*10 + 16*0]
    movups  xmm7, [rdx + 8*10 + 16*1]
    movups  xmm8, [rdx + 8*10 + 16*2]
    movups  xmm9, [rdx + 8*10 + 16*3]
    movups  xmm10, [rdx + 8*10 + 16*4]
    movups  xmm11, [rdx + 8*10 + 16*5]
    movups  xmm12, [rdx + 8*10 + 16*6]
    movups  xmm13, [rdx + 8*10 + 16*7]
    movups  xmm14, [rdx + 8*10 + 16*8]
    movups  xmm15, [rdx + 8*10 + 16*9]

    ; Set SSE & x87 FPU
    ldmxcsr [rdx + 8*34]
    fldcw   [rdx + 8*34 + 4]

    ; Move user data argument into rcx
    mov     rcx, [rdx + 8*35]
    
    ; Should return to the address set with Get/SwapContext
    mov     rax, [rdx + 8*0]

    ; Push RIP to stack for RET.
    push    rax

    ; Return
    xor eax, eax
    ret
FiberSwapContext ENDP

FiberThreadGetFPState PROC
    ; Save SSE & x87 FPU
    stmxcsr [rcx + 0]
    fnstcw  [rcx + 4]

    ; Return
    xor eax, eax
    ret
FiberThreadGetFPState ENDP
END