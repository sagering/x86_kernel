
void switch_to_user_mode();

int main()
{
  switch_to_user_mode();
}

typedef unsigned int u32;
struct ir_frame;

__attribute__((interrupt)) void ir13(struct ir_frame* f)
{
  u32 volatile error = 0xff;
  asm volatile ("mov -0x8(%%ebp), %0" : "=r"(error) );
}

__attribute__((naked)) void switch_to_user_mode()
{
  // IRET expectes following data on the stack;
  // 1. stack segment selector ss
  // 2. stack pointer after IRET
  // 3. EFLAGS
  // 4. code segmenent selector cs
  // 5. instruction pointer EIP
  //
  // This code here turn interrupts on again by setting the EFLAGS.IF
  // pop %eax
  // or $0x200, %eax
  // push %eax

  asm volatile ("   \
    cli;            \
    mov $0x20, %ax; \
    mov %ax, %ds;   \
    mov %ax, %es;   \
    mov %ax, %fs;   \
    mov %ax, %gs;   \
                    \
    mov %esp, %eax; \
                    \
    pushl $0x10;    \
    pushl %eax;     \
    pushf;          \
    pop %eax;       \
    or $0x200, %eax;\
    push %eax;      \
    pushl $0x18;    \
    push $main;     \
    iret;           \
  ");
}


