SECTIONS
{
  .text  0x00000000 : {
    *(.text)
  }
  .data  : {
    *(.data)
  }
  .bss   : {
    *(.bss)
  }
   _END = .;
   _end = .;
   _edata = .;
}
