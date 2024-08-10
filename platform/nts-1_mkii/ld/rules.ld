/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 * File: rules.ld
 *
 * Linker Rules
 */

/* ----------------------------------------------------------------------------- */
/* Define output sections */

PHDRS
{
  headers PT_PHDR PHDRS;
  /* interp PT_INTERP; */
  text PT_LOAD FILEHDR PHDRS;
  unit_header PT_LOAD;
  data PT_LOAD;
  dynamic PT_DYNAMIC;
}

SECTIONS
{
  . = SIZEOF_HEADERS;
  
  .hash : ALIGN(4) SUBALIGN(4)
  {
    KEEP(*(.hash))
  } :text
  
  .dynsym : ALIGN(4) SUBALIGN(4)
  {
    KEEP(*(.dynsym))
  } :text
  
  .dynstr : ALIGN(4) SUBALIGN(4)
  {
    KEEP(*(.dynstr))
  } :text
    
  .rel.dyn : ALIGN(4) SUBALIGN(4)
  {
    KEEP(*(.rel.dyn))
  } :text
    
  .rel.plt : ALIGN(4) SUBALIGN(4)
  {
    KEEP(*(.rel.plt))
  } :text
      
  .text : ALIGN(4) SUBALIGN(4)
  {
    *(.text)
    *(.text.*)
    /* *(.glue_7) */
    /* *(.glue_7t) */
    /* *(.gcc*) */
    
    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    *(.rodata)
    *(.rodata.*)
    
  } :text
  
  /* .rodata : ALIGN(4) SUBALIGN(4) */
  /* { */
  /*   __rodata_start__ = .; */
  /*   *(.rodata) */
  /*   *(.rodata.*) */
  /*   __rodata_end__ = .; */
  /* } :text */

  .unit_header : ALIGN(8) SUBALIGN(8)
  {
    . = ALIGN(8);
    KEEP(*(.unit_header))
  } :unit_header

  .preinit_array : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } :data
  
  .init_array : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);
    /* KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*))) */
    /* KEEP(*(.init_array .ctors)) */
  } :data

  .dynamic : ALIGN(8) SUBALIGN(8)
  {
    . = ALIGN(8);
    *(.dynamic)
  } :data :dynamic

  .got : ALIGN(4) SUBALIGN(4)
  {
    *(.got.plt)
    *(.igot.plt)
    *(.got)
    *(.igot)
  } :data
  
  .data ALIGN(4) : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)
    . = ALIGN(4);
    __data_end__ = .;
  } :data
  
  .bss (NOLOAD) : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } :data

  /* Needed for libnosys */
  .stack (NOLOAD) : ALIGN(16)
  {
    . = ALIGN(16);
    __HEAP = .;
    _end = .;
    end = .;
    . += __heap_size;
    __HEAP_END = .;
    _stack_end = .;
    . +=  __stack_size;
    _stack_addr = .;
    __stack = .;
    __SP_INIT = .;
    . += 4;
   } :data

  /* Exception sections */
  /* .ARM.extab : ALIGN(4) SUBALIGN(4) */
  /* { */
  /*   . = ALIGN(4); */
  /*   *(.ARM.extab* .gnu.linkonce.armextab.*) */
  /* } */
  
  /* .ARM.exidx */
  /* {  */
  /*   *(.ARM.exidx* .gnu.linkonce.armexidx.*) */
  /* } */
  /* } > SRAM */
  
  /* .eh_frame_hdr : ALIGN(4) SUBALIGN(4) */
  /* { */
  /*   . = ALIGN(4); */
  /*   *(.eh_frame_hdr) */
  /*   . = ALIGN(4); */
  /* } */
  /* } > SRAM */
  
  /* .eh_frame : ALIGN(4) SUBALIGN(4) ONLY_IF_RO */
  /* { */
  /*   . = ALIGN(4); */
  /*   *(.eh_frame) */
  /*   . = ALIGN(4); */
  /* } */
  /* } > SRAM */

  /*
  /DISCARD/
  {
    libc.a   ( * )
    libm.a   ( * )
    libgcc.a ( * )
  }
  //*/

  /* .ARM.attributes 0 : { *(.ARM.attributes) } //*/

  /DISCARD/ :
  { 
    *(.fini_array.* .fini_array .dtors.* .dtors)
    *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*)
  }
}
