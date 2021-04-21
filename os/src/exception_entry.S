.altmacro    # %exp
# 寄存器宽度对应的字节数
.set    REG_SIZE, 8
# Context 的大小
.set    CONTEXT_SIZE, 36

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

    # 保存 x3 - x31
    .set    n, 3
    .rept   29
        N %n SAVE
        .set    n, n + 1
    .endr

    
    # 若从内核态进入中断，此时 sscratch 为内核栈地址（未分配context空间）， 先保存再清零
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

# 高地址        
#  ------------- SP+36*8
#  scause
#  -------------
#  stval
#  -------------
#  sepc   
#  -------------      
#  sstatus    
#  ------------- SP+32*8
#  x31        
#  ------------- SP+31*8
#  ...
#  -------------
#  x2         
#  ------------- SP+2*8
#  x1          
#  ------------- SP+1*8 
#  x0          
#  ------------- SP 
# 低地址


    # 调用 e_dispatch，传入参数
    mv      a0, sp
    # scause: Scause
    csrr    a1, scause
    # stval: usize
    csrr    a2, stval
    jal  e_dispatch

    .globl __restore

# 离开中断
# 从 Context 中恢复所有寄存器，并跳转addi s0, sp, 36 * XLENB至 Context 中 sepc 的位置

__restore:

    # a0 从哪里 赋值
    # sp ？同  a0依然在内核栈顶
    # 为了下降特权级到 U， 直接调用 __restore
    # 在 中断处理 完后 返回 U 或者 S
    mv sp, a0

    # 另一种方式判断是从内核态还是用户态进入中断
    # 00 U, 01 S, 11 M
    # 如果从内核态进入中断， sstatus 的 SPP 位被硬件设为 1
    # 如果从用户态进入中断， sstatus 的 SPP 位被硬件设为 0    或者 下降特权级到用户态
    LOAD    s1, 32

    andi    s0, s1, 1 << 8
    bnez    s0, to_k

to_u:
    addi    s0, sp, REG_SIZE*CONTEXT_SIZE
    csrw    sscratch, s0
    # 如果 返回用户态， 需要设置sscratch 为内核 栈 地址（未分配context空间）
    # 如果 返回内核态， sscratch要清零， 在saveall中已经清零 ？

to_k:


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

    # 恢复 x3 至 x31
    .set    n, 3
    .rept   29
        N %n LOAD
        .set    n, n + 1
    .endr

    # 恢复 sp（x2）这里最后恢复
    # 如果 返回 用户态 或 下降特权级到用户态 ， 该地址保存 用户栈指针
    # 如果 返回 内核态 ， 该地址保存 内核栈指针
    LOAD    x2, 2
text:
    sret