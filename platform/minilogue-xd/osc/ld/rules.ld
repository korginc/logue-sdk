/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
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

SECTIONS
{
  
  .hooks : ALIGN(16) SUBALIGN(16)
  {
    . = ALIGN(4);
    _hooks_start = .;
    KEEP(*(.hooks))
    . = ALIGN(4);
    _hooks_end = .;
  } > SRAM
  
  /* Constructors */
  .preinit_array : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    PROVIDE(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    . = ALIGN(4);
    PROVIDE(__preinit_array_end = .);
  } > SRAM

  /* Constructors */
  .init_array : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    PROVIDE(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    . = ALIGN(4);
    PROVIDE(__init_array_end = .);
  } > SRAM

  /* Destructors */
  .fini_array : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    PROVIDE(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    . = ALIGN(4);
    PROVIDE(__fini_array_end = .);
  } > SRAM
  
  /* Common Code */
  .text : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    _text_start = .;
    *(.text)
    *(.text.*)
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.gcc*)
    . = ALIGN(4);
    _text_end = .;
  } > SRAM

  /* Constants and strings */
  .rodata : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    _rodata_start = .;
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);
    _rodata_end = .;
  } > SRAM

  /* Read-write data */  
  .data ALIGN(8) : ALIGN(8) SUBALIGN(8)
  {
    . = ALIGN(8);
    _data_start = .;
    *(.data)
    *(.data.*)
    . = ALIGN(8);
    _data_end = .;
  } > SRAM  

  /* Uninitialized variables */
  .bss (NOLOAD) : ALIGN(4)
  {
    . = ALIGN(4);
    _bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } > SRAM

  /* Exception sections */
  .ARM.extab : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    __extab_start = .;
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
    __extab_end = .;
  } > SRAM
  
  .ARM.exidx : ALIGN(4) SUBALIGN(4)
  { /* Note: Aligning when there's no content for this section throws a warning. Looks like a linker bug. */
    /* . = ALIGN(4); */
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    /* . = ALIGN(4); */
    __exidx_end = .;
  } > SRAM
  
  .eh_frame_hdr : ALIGN(4) SUBALIGN(4)
  {
    . = ALIGN(4);
    _eh_frame_hdr_start = .;
    *(.eh_frame_hdr)
    . = ALIGN(4);
    _eh_frame_hdr_end = .;
  } > SRAM
  
  .eh_frame : ALIGN(4) SUBALIGN(4) ONLY_IF_RO
  {
    . = ALIGN(4);
    _eh_frame_start = .;
    *(.eh_frame)
    . = ALIGN(4);
    _eh_frame_end = .;
  } > SRAM
    
  /*
  /DISCARD/
  {
    libc.a   ( * )
    libm.a   ( * )
    libgcc.a ( * )
  }
  //*/
  
  /* .ARM.attributes 0 : { *(.ARM.attributes) } //*/
}
