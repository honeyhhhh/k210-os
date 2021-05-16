OUTPUT_ARCH(riscv)
ENTRY(_start)
BASE_ADDRESS = 0x80200000;

SECTIONS
{
    . = BASE_ADDRESS;
    skernel = .;

    
    .text : {
        stext = .;
        *(.text.entry)
        . = ALIGN(4K);
        strampoline = .;
        *(.text.trampoline);
        . = ALIGN(4K);
        *(.text .text.*)
        . = ALIGN(4K);
        etext = .;
    }

    
    .rodata : {
        srodata = .;
        *(.rodata .rodata.*)
       . = ALIGN(4K);
        erodata = .;
    }

    
    .data : {
        sdata = .;
        *(.data .data.*)
        . = ALIGN(4K);
        edata = .;
    }

    sbss_with_stack = .;
    _heap_allocator = .;
    .bss : {
        *(.bss.stack)
        sbss = .;
        *(.bss .bss.*)
        . = ALIGN(4K);
        ebss = .;
    }

    ekernel = .;

    
}