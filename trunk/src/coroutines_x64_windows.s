; Copyright (c) 2008-2013, Helios (helios.vmg@gmail.com)
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;     * Redistributions of source code must retain the above copyright notice,
;       this list of conditions and the following disclaimer.
;     * Redistributions in binary form must reproduce the above copyright
;       notice, this list of conditions and the following disclaimer in the
;       documentation and/or other materials provided with the distribution.
;     * The name of the author may not be used to endorse or promote products
;       derived from this software without specific prior written permission.
;     * Products derived from this software may not be called "ONSlaught" nor
;       may "ONSlaught" appear in their names without specific prior written
;       permission from the author.
;
; THIS SOFTWARE IS PROVIDED BY HELIOS "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL HELIOS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.


; This file meant to be assembled by NASM, the Netwide Assembler.


section .text
global initialize_stack
global coroutine_resume
global coroutine_yield

initialize_stack:
	mov		rax, rcx
	
	; switching context...
	mov		[rax+8], rsp
	mov		rsp, [rax+16]
	; now in virtual stack
	
	mov		rcx, [rax+24]

	lea		r9, [rel coroutine_entry_point]
	push	r9
	
	push	0
	push	rcx
	mov		r9, 12
loop_begin:
	push	0
	dec		r9
	jnz		loop_begin
loop_end:
	
	; switching context...
	mov [rax+16], rsp
	mov rsp, [rax+8]
	; now in real stack
	
	ret

coroutine_resume:
	mov		rax, rcx

	mov		rcx, [rax+24]
	mov		[rcx], rbp
	mov		rdx, [rsp]
	mov		[rcx+8], rdx
	
	push	rbp
	mov		rbp, rsp

	mov		rdx, [rax]
	
	push	rbp
	push	rbx
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	sub		rsp, 32
	movdqu	[rsp+16], xmm6
	movdqu	[rsp], xmm7
    push	rsi
    push	rdi

	; switching context...
	mov		[rax+8], rsp
	mov		rsp, [rax+16]
	; now in virtual stack
	
	pop		rdi
	pop		rsi
	movdqu	xmm7, [rsp]
	movdqu	xmm6, [rsp+16]
	add		rsp, 32
	pop		r15
	pop		r14
	pop		r13
	pop		r12
	pop		r11
	pop		r10
	pop		rbp
	pop		rbx
	
	pop		rcx
	jmp		rcx
	; function "never returns"

coroutine_entry_point:
	lea		r9, [rel coroutine_entry_point2]
	push	r9
coroutine_entry_point2:
	mov		rcx, rax
	call	rdx
	pop		rax
	lea		r9, [rel coroutine_entry_point2]
	push	r9
	jmp		coroutine_yield2

coroutine_yield:
	mov		rax, rcx
coroutine_yield2:
	push	rbx
    push	rbp
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	sub		rsp, 32
	movdqu	[rsp+16], xmm6
	movdqu	[rsp], xmm7
	push	rsi
	push	rdi
	
	; switching context...
	mov [rax+16], rsp
	mov rsp, [rax+8]
	; now in real stack
		
    pop		rdi
    pop		rsi
	movdqu	xmm7, [rsp]
	movdqu	xmm6, [rsp+16]
	add		rsp, 32
	pop		r15
	pop		r14
	pop		r13
	pop		r12
	pop		r11
	pop		r10
	pop		rbx
	pop		rbp

	mov		rsp, rbp
	pop		rbp
	
	ret
