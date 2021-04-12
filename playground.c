
#define u32 unsigned int

extern u32 eip;
extern u32 cs;
extern u32 eflags;
extern u32 esp;
extern u32 ss;

__attribute__((naked)) void something() { int j = 0; ++j; }
__attribute__((naked)) void ir13()
{
  asm volatile("push %0" :: "r"(eip));
  asm volatile("push %0" :: "r"(cs));
  asm volatile("push %0" :: "r"(eflags & 0x200));
  asm volatile("push %0" :: "r"(esp));
  asm volatile("push %0" :: "r"(ss));

  asm volatile("xor %eax, %eax;");
  asm volatile("xor %ebx, %ebx;");
  asm volatile("xor %ecx, %ecx;");
  asm volatile("xor %edx, %edx;");
  asm volatile("xor %edi, %edi;");
  asm volatile("xor %esi, %esi;");
  asm volatile("xor %ebp, %ebp;");

  something();
}

