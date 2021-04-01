ENTRY(start)
OUTPUT_FORMAT(binary)

SECTIONS
{
. = 0x8000;
.text : { *(.text) }
.data : { *(.data) }
.bss : { *(.bss) }

/DISCARD/ : 
{
  *(.comment)
  *(.eh_frame)
  *(.rel.eh_frame)
  *(.note.*)
}

}
