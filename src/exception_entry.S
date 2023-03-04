.altmacro    # %exp
# 寄存器宽度对应的字节数
.set    REG_SIZE, 8
# Context 的大小
.set    CONTEXT_SIZE, 39

# reg -> sp:
.macro SAVE reg, offset
    sd  \reg, \offset*REG_SIZE(sp)
.endm

# sp: -> regs
.macro LOAD reg, offset
    ld  \reg, \offset*REG_SIZE(sp)
.endm

.macro N reg_no OP
    OP x\reg_no reg_no
.endm



#-------------------------------------------
    .section .text.trampoline

    .align 2
    .globl __saveall

__saveall:

    csrrw   sp, sscratch, sp


    # 保存 x1
    SAVE    x1, 1
    SAVE    x3, 3
    # 保存 x5 - x31
    .set    n, 5
    .rept   27
        N %n SAVE
        .set    n, n + 1
    .endr


    csrr    s0, sscratch
    SAVE    s0, 2

    # 保存 CSR 
    csrr    s1, sstatus
    csrr    s2, sepc
    csrr    s3, stval
    csrr    s4, scause
    SAVE    s1, 32
    SAVE    s2, 33
    SAVE    s3, 34
    SAVE    s4, 35

    # 保存 kernel_satp 到t0
    LOAD    t0, 36
    # 保存 trap_handle 到t1
    LOAD    t1, 38
    # 保存 kernel_sp 到 sp   ?? 内核　boot_stack_top
    LOAD    sp, 37


    csrw    satp, t0
    fence
    fence.i
    sfence.vma zero,zero
    fence
    fence.i


    jr t1

    .globl __restore



__restore:

    # a0: *TrapContext in user space(Constant); a1: user space token

    csrw    satp, a1
    fence
    fence.i
    sfence.vma zero,zero
    fence
    fence.i




    csrw    sscratch, a0
    mv      sp, a0



    LOAD    s1, 32

    # 恢复 CSR
    LOAD    s2, 33
    LOAD    s3, 34
    LOAD    s4, 35
    csrw    sstatus, s1
    csrw    sepc, s2
    csrw    stval, s3
    csrw    scause, s4

    # 恢复 x1
    LOAD    x1, 1

    LOAD    x3, 3
    # 恢复 x5 至 x31
    .set    n, 5
    .rept   27
        N %n LOAD
        .set    n, n + 1
    .endr

    LOAD    x2, 2

    sret