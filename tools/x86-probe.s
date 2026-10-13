# Linux x86_64 execution probe, independent of shared libraries.
# Exit 0 only after arithmetic, a conditional branch, and stdout write succeed.
.global _start
.section .text
_start:
    mov $7, %rax
    imul $6, %rax
    cmp $42, %rax
    jne failure
    mov $1, %rax
    mov $1, %rdi
    lea message(%rip), %rsi
    mov $message_end-message, %rdx
    syscall
    cmp $message_end-message, %rax
    jne failure
    xor %rdi, %rdi
    jmp finish
failure:
    mov $1, %rdi
finish:
    mov $60, %rax
    syscall
.section .rodata
message:
    .ascii "School Tycoon x86 execution probe: PASS\n"
message_end:
.section .note.GNU-stack,"",@progbits
