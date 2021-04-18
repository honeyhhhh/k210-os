.altmacro    # %exp
# 寄存器宽度对应的字节数
.set    REG_SIZE, 8
# Context 的大小
.set    CONTEXT_SIZE, 36


.macro SAVE reg, offset
    sd  \reg, \offset*REG_SIZE(sp)
.endm

.macro LOAD reg, offset
    ld  \reg, \offset*REG_SIZE(sp)
.endm

.macro N reg_no OP
    OP x\reg_no reg_no
.endm



#-------------------------------------------
    .section .text
 .align 2
    .globl __saveall
# 进入中断
# 保存 Context 并且进入的中断处理函数
__saveall:

    # 判断是从内核态还是 用户态进入 中断
    # 原子操作交换sp和sscratch
    csrrw   sp, sscratch, sp

    # 如果SP = 0 说明从内核态进入中断，无需切换栈，将 sscratch 的值读回 sp ，此时 sp,sscratch 均保存内核栈
    bnez    sp, ir_from_u

ir_from_k:
    csrr    sp, sscratch
    #csrrw  sp, sscratch, sp

ir_from_u:

    # 在栈上开辟 Context 所需的空间
    addi    sp, sp, -1*CONTEXT_SIZE*REG_SIZE

    # 保存 x1
    SAVE    x1, 1

    # 保存 x3 至 x31
    .set    n, 3
    .rept   29
        N %n SAVE
        .set    n, n + 1
    .endr

    
    # 若从内核态进入中断，此时 sscratch 为内核栈地址， 需要清零
    # 若从用户态进入中断，此时 sscratch 为用户栈地址
    # sscratch-> s0   再 x0 -> sscratch
    csrrw   s0, sscratch, x0
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


    # 调用 e_dispatch，传入参数
    mv      a0, sp
    # scause: Scause
    csrr    a1, scause
    # stval: usize
    csrr    a2, stval
    jal  e_dispatch

    .globl __restore

# 离开中断
# 从 Context 中恢复所有寄存器，并跳转至 Context 中 sepc 的位置
__restore:
    # 恢复 CSR
    LOAD    s1, 32
    LOAD    s2, 33
    csrw    sstatus, s1
    csrw    sepc, s2

    # 恢复通用寄存器
    LOAD    x1, 1
    # 恢复 x3 至 x31
    .set    n, 3
    .rept   29
        N %n LOAD
        .set    n, n + 1
    .endr

    # 恢复 sp（又名 x2）这里最后恢复是为了上面可以正常使用 LOAD 宏
    LOAD    x2, 2
    sret