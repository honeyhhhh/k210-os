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
# 进入中断
# 保存 Context 并且进入的中断处理函数
__saveall:
    #li t0,
    #jr t0
    # 判断是从内核态还是 用户态进入 中断
    # 原子操作交换sp和sscratch
    csrrw   sp, sscratch, sp

    # 如果SP = 0 说明从内核态进入中断，无需切换栈，将 sscratch 的值读回 sp ，此时 sp,sscratch 均保存内核栈
    # bnez    sp, ir_from_u

#ir_from_k:
    # csrr    sp, sscratch
    #csrrw  sp, sscratch, sp
    # 在栈上开辟 Context 所需的空间
    # addi    sp, sp, -1*CONTEXT_SIZE*REG_SIZE

    # 保存 x1
    # SAVE    x1, 1

    # 保存 x3 - x31
    # .set    n, 3
    # .rept   29
    #     N %n SAVE
    #    .set    n, n + 1
    #.endr

    # 若从内核态进入中断，此时 sscratch 为内核栈地址（未分配context空间）， 先保存再清零
    # sscratch-> s0   再 x0 -> sscratch
    # csrrw   s0, sscratch, x0
    # SAVE    s0, 2

    # 保存 CSR 
    # csrr    s1, sstatus
    # csrr    s2, sepc
    # csrr    s3, stval
    # csrr    s4, scause
    # SAVE    s1, 32
    # SAVE    s2, 33
    # SAVE    s3, 34
    # SAVE    s4, 35

    # 调用 e_dispatch，传入参数
    # mv      a0, sp
    # scause: Scause
    # csrr    a1, scause
    # stval: usize
    # csrr    a2, stval
    # jal  e_dispatch

    # j __restore

#ir_from_u:
    # 当应用 Trap 进入内核的时候，硬件会设置一些 CSR 并在 S 特权级下跳转到 __alltraps 保存 Trap 上下文。
    # 此时 sp 寄存器仍指向用户栈，但 sscratch 则被设置为指向应用地址空间中存放 Trap 上下文的位置，实际在次高页面。 
    # 随后，就像之前一样，我们 csrrw 交换 sp 和 sscratch ，并基于指向 Trap 上下文位置的 sp 开始保存通用 寄存器和一些 CSR 
    # 到这里，我们就全程在应用地址空间中完成了保存 Trap 上下文的工作。
    # 注意　如果从用户态进入中断　以前的上下文是保存在内核栈的，正好以前的sscratch指向内核栈，而现在指向虚拟地址空间的次高页面

    # 接下来该考虑切换到内核地址空间并跳转到 trap handler 了。
    # 我们将内核地址空间的 token 载入到 t0 寄存器中， 
    # 然后将 trap handler 入口点的虚拟地址载入到 t1 寄存器中
    # 我们直接将 sp 修改为应用内核栈顶的地址。 这三条信息均是内核在初始化该应用的时候就已经设置好的。
    # 然后我们将 satp 修改为内核地址空间的 token 并使用 sfence.vma 刷新快表，这就切换到了内核地址空间。
    # 最后我们通过 jr 指令跳转到 t1 寄存器所保存的 trap handler 入口点的地址。注意这里我们不能像之前的章节那样直接 call trap_handler



    # 保存 x1
    SAVE    x1, 1
    SAVE    x3, 3
    # 保存 x5 - x31
    .set    n, 5
    .rept   27
        N %n SAVE
        .set    n, n + 1
    .endr

    
    # 若从内核态进入中断，此时 sscratch 为
    # 若从用户态进入中断，此时 sscratch 为
    # sscratch-> s0   再 x0 -> sscratch
    # csrrw   s0, sscratch, x0
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

    # 切换到 内核地址空间
    # li t0,0x800000000008005d
    csrw    satp, t0
    fence
    fence.i
    sfence.vma
    fence
    fence.i

    # li t1,0x000000008002305e
    #jr t1

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
#  x1          la      t0,
#  ------------- SP+1*8 
#  x0          
#  ------------- SP 
# 低地址



    # a0 is no longer valid, since the kernel page
    # table does not specially map p->tf.
    # jump to usertrap(), which does not return

    # jalr x0，t1，0
    # li t1,0x00000000800230a8
    jr t1

    .globl __restore

# 离开中断

__restore:

    # a0 从哪里 赋值
    # sp ？同  a0依然在内核栈顶
    # 为了下降特权级到 U， 直接调用 __restore
    # 在 中断处理 完后 返回 U 或者 S
    # mv sp, a0

    # 另一种方式判断是从内核态还是用户态进入中断
    # 00 U, 01 S, 11 M
    # 如果从内核态进入中断， sstatus 的 SPP 位被硬件设为 1
    # 如果从用户态进入中断， sstatus 的 SPP 位被硬件设为 0    或者 下降特权级到用户态


    # 第一个是 Trap 上下文在应用 地址空间中的位置，这个对于所有的应用来说都是相同的，由调用规范在 a0 寄存器中传递；
    # 第二个则是即将回到的应用的地址空间 的 token ，在 a1 寄存器中传递。
    # 由于 Trap 上下文是保存在应用地址空间中的，所以我们先切换回应用地址空间。
    # 再我们将传入的 Trap 上下文位置保存在 sscratch 寄存器中，这样 __alltraps 中才能基于它将 Trap 上下文 保存到正确的位置。
    # 然后将 sp 修改为 Trap 上下文的位置，后面基于它恢复各通用寄存器和 CSR。
    # a0: *TrapContext in user space(Constant); a1: user space token

    csrw    satp, a1
    fence
    fence.i
    sfence.vma
    fence
    fence.i




    #　addi    s0, sp, REG_SIZE*CONTEXT_SIZE
    #　csrw    sscratch, s0

    csrw    sscratch, a0
    mv      sp, a0
    # now sp points to TrapContext in user space, start restoring based on it

    # 如果 返回用户态， 需要设置sscratch 为内核 栈 地址（未分配context空间），而现在需要设置为　Ｔｒａｐ上下文在用户地址空间的位置
    # 如果 返回内核态， sscratch要清零， 在saveall中已经清零 ？


    LOAD    s1, 32

    #andi    s0, s1, 1 << 8
    #bnez    s0, to_k

#to_u:
    #li s2, 0xfffffffffffff000


#to_k:


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

    # 恢复 sp（x2）这里最后恢复
    # 如果 返回 用户态 或 下降特权级到用户态 ， 该地址保存 用户栈指针
    # 如果 返回 内核态 ， 该地址保存 内核栈指针(未分配ＣＯＮＴＥＸＴ)
    LOAD    x2, 2

    sret