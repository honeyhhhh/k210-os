#ifndef _RISCV_ASM_H
#define _RISCV_ASM_H

#ifdef __ASSEMBLER__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif



#define CSR_SSTATUS		0x100
#define CSR_SIE			0x104
#define CSR_STVEC		0x105
#define CSR_SCOUNTEREN		0x106
#define CSR_SSCRATCH		0x140
#define CSR_SEPC		0x141
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143
#define CSR_SIP			0x144
#define CSR_SATP		0x180

#define CSR_MSTATUS		0x300
#define CSR_MISA		0x301
#define CSR_MIE			0x304
#define CSR_MTVEC		0x305
#define CSR_MSCRATCH		0x340
#define CSR_MEPC		0x341
#define CSR_MCAUSE		0x342
#define CSR_MTVAL		0x343
#define CSR_MIP			0x344
#define CSR_PMPCFG0		0x3a0
#define CSR_PMPADDR0		0x3b0
#define CSR_MHARTID		0xf14

#define MSTATUS_UIE         0x00000001
#define MSTATUS_SIE         0x00000002
#define MSTATUS_HIE         0x00000004
#define MSTATUS_MIE         0x00000008
#define MSTATUS_UPIE        0x00000010
#define MSTATUS_SPIE        0x00000020
#define MSTATUS_HPIE        0x00000040
#define MSTATUS_MPIE        0x00000080
#define MSTATUS_SPP         0x00000100
#define MSTATUS_HPP         0x00000600
#define MSTATUS_MPP         0x00001800
#define MSTATUS_FS          0x00006000
#define MSTATUS_XS          0x00018000
#define MSTATUS_MPRV        0x00020000
#define MSTATUS_PUM         0x00040000
#define MSTATUS_MXR         0x00080000
#define MSTATUS_VM          0x1F000000
#define MSTATUS32_SD        0x80000000
#define MSTATUS64_SD        0x8000000000000000

#define SSTATUS_UIE         0x00000001
#define SSTATUS_SIE         0x00000002
#define SSTATUS_UPIE        0x00000010
#define SSTATUS_SPIE        0x00000020
#define SSTATUS_SPP         0x00000100
#define SSTATUS_FS          0x00006000
#define SSTATUS_XS          0x00018000
#define SSTATUS_PUM         0x00040000
#define SSTATUS32_SD        0x80000000
#define SSTATUS64_SD        0x8000000000000000








#ifndef __ASSEMBLER__

// csrrw dst, csr, src（CSR Read Write）t = CSRs[csr]; CSRs[csr] = x[rs1]; x[rd] = t
#define csr_swap(csr, val)                                              \
	({                                                              \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrw %0, " __ASM_STR(csr) ", %1" \
				     : "=r"(__v)                        \
				     : "rK"(__v)                        \
				     : "memory");                       \
		__v;                                                    \
	})


// 等同于 csrrs rd, csr, x0	
#define csr_read(csr)                                           \
	({                                                      \
		register unsigned long __v;                     \
		__asm__ __volatile__("csrr %0, " __ASM_STR(csr) \
				     : "=r"(__v)                \
				     :                          \
				     : "memory");               \
		__v;                                            \
	})

// 等同于csrrs x0, csr, rs1
#define csr_write(csr, val)                                        \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrw " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

// csrrs rd, csr, rs1     t = CSRs[csr]; CSRs[csr] = t | x[rs1]; x[rd] = t 
#define csr_read_set(csr, val)                                          \
	({                                                              \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrs %0, " __ASM_STR(csr) ", %1" \
				     : "=r"(__v)                        \
				     : "rK"(__v)                        \
				     : "memory");                       \
		__v;                                                    \
	})

// 将 CSR 寄存器中指定的位置 1，csrs 使用通用寄存器作为 mask   等同于 csrrs x0, csr, rs1
#define csr_set(csr, val)                                          \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrs " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

// csrrc rd, csr, rs1 	t = CSRs[csr]; CSRs[csr] = t &~x[rs1]; x[rd] = t 
#define csr_read_clear(csr, val)                                        \
	({                                                              \
		unsigned long __v = (unsigned long)(val);               \
		__asm__ __volatile__("csrrc %0, " __ASM_STR(csr) ", %1" \
				     : "=r"(__v)                        \
				     : "rK"(__v)                        \
				     : "memory");                       \
		__v;                                                    \
	})

// 将 CSR 寄存器中指定的位清零，csrc 使用通用寄存器rs1作为 mask  等同于 csrrc(i) x0, csr, rs1
#define csr_clear(csr, val)                                        \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrc " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})



#endif

#endif
