ENTRY(start)
OUTPUT_FORMAT(binary)

SECTIONS
{
. = 0x8000;
_text_start = .;
.text : { *(.text) }
_text_end = .;
_data_start = .;
.data : { *(.data) *(.rodata) }
_data_end = .;
_bss_start = .;
.bss : { *(.bss) }
_bss_end = .;

/DISCARD/ : 
{
  *(.comment)
  *(.eh_frame)
  *(.rel.eh_frame)
  *(.note.*)
}

}
