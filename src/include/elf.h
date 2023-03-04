// Format of an ELF executable file

#ifndef _ELF_H
#define _ELF_H
#include "stddef.h"


// 顺序
//ELF header
//若干个 program header
//程序各个段的实际数据
//若干的 section header



#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
    uint magic;  // must equal ELF_MAGIC, 魔数 (Magic) 独特的常数，存放在 ELF header 的一个固定位置。当加载器将 ELF 文件加载到内存之前，通常会查看 该位置的值是否正确，来快速确认被加载的文件是不是一个 ELF 。
    uint8_t elf[12];
    uint16_t type;    // EXEC (Executable file)
    uint16_t machine;
    uint version;
    uint64_t entry;  //可执行文件的入口点
    uint64_t phoff;     // Start of program headers  就在ｅｌｆ之后
    uint64_t shoff; //Start of section headers
    uint flags;
    uint16_t ehsize;        //Size of this header
    uint16_t phsize;     // Size of program headers
    uint16_t phnum;     // Number of program headers
    uint16_t shsize;    // Size of section headers
    uint16_t shnum;     // Number of section headers
    uint16_t shstrndx;  // Section header string table index
};




// 除了 ELF header 之外，还有另外两种不同的 header
// 分别称为 program header 和 section header， 它们都有多个。
// ELF header 中给出了其他两种header 的大小、在文件中的位置以及数目。


// 每个 section header 则描述一个段的元数据。
// 比如代码段 .text 需要被加载到地址 0x5070 ,大小 208067 字节。 它们分别由元数据的字段 Offset、 Size 和 Address 给出

// Program section header
struct proghdr {
    uint32_t type;
    uint32_t flags;
    uint64_t off;       //从文件头到该段第一个字节的偏移。
    uint64_t vaddr;     //此成员给出段的第一个字节将被放到内存中的虚拟地址。
    uint64_t paddr;     
    uint64_t filesz;   //此成员给出段在文件映像中所占的字节数 
    uint64_t memsz; //此成员给出段在内存映像中占用的字节数
    uint64_t align;
};

// Values for Proghdr type
// 可加载段
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4


#endif