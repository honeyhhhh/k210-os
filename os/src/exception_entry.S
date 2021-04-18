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
    # 在栈上开辟 Context 所需的空间
    addi    sp, sp, -1*CONTEXT_SIZE*REG_SIZE

    # 保存通用寄存器，除了 x0（固定为 0）
    SAVE    x1, 1
    # 将原来的 sp（x2）写入 2 位置
    addi    x1, sp, 34*8
    SAVE    x1, 2
    # 保存 x3 至 x31
    .set    n, 3
    .rept   29
        N %n SAVE
        .set    n, n + 1
    .endr

    # 取出 CSR 并保存
    csrr    s1, sstatus
    csrr    s2, sepc
    SAVE    s1, 32
    SAVE    s2, 33

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