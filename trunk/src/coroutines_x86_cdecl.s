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
global _initialize_stack
global _coroutine_resume
global _coroutine_yield

_initialize_stack:
	mov		eax, [esp+4]
	
	; switching context...
	mov		[eax+4], esp
	mov		esp, [eax+8]
	; now in virtual stack
	
	mov		ecx, [eax+12]

	push	coroutine_entry_point
	
	push	0
	push	ecx
	push	0
	push	0
	
	; switching context...
	mov [eax+8], esp
	mov esp, [eax+4]
	; now in real stack
	
	ret

_coroutine_resume:
	mov		eax, [esp+4]

	mov		ecx, [eax+12]
	mov		[ecx], ebp
	mov		edx, [esp]
	mov		[ecx+4], edx
	
	push	ebp
	mov		ebp, esp

	mov		edx, [eax]
	
	pusha

	; switching context...
	mov		[eax+4], esp
	mov		esp, [eax+8]
	; now in virtual stack
	
	pop		edi
	pop		esi
	pop		ebp
	pop		ebx
	
	pop		ecx
	jmp		ecx
	; function "never returns"

coroutine_entry_point:
	push	coroutine_entry_point2
coroutine_entry_point2:
	push	eax
	call	edx
	pop		eax
	push	coroutine_entry_point2
	jmp		coroutine_yield2

_coroutine_yield:
	mov		eax, [esp+4]
coroutine_yield2:
	push	ebx
	push	ebp
	push	esi
	push	edi
	;push	ecx
	
	; switching context...
	mov [eax+8], esp
	mov esp, [eax+4]
	; now in real stack
		
	popa

	mov		esp, ebp
	pop		ebp
	ret
