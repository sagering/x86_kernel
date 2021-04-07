void start(); // has to be first symbol

typedef int i32;
typedef short int i16;
typedef char i8;

typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char u8;

void clear_screen();
void put_char(char);

void print(char const*);
void print_u32();
void print_u32_hex();

void init_gdt();
void init_paging();
void init_interrupt_handlers();

void switch_to_user_mode();

void start() 
{
  clear_screen();

  print("Init gdt...\n");
  init_gdt();
  print("Gdt initialized!\n");

  print("Init interrupts...\n");
  init_interrupt_handlers();
  print("Interrupts initialized!\n");

  print("Init paging...\n");
  init_paging();
  print("Paging initialized!\n");

  print("Switching to user mode...\n");
  switch_to_user_mode();

  print("Not reached");
}

__attribute__((naked)) void user_mode()
{
  int i =0 ;
  while(1) { if(i > 1000000) { i = 0; print("Hello from userland!\n"); } ++i; }
}

extern u32 _bss_end;

void memset(u8* dst, u8 val, u32 size)
{
  while(size--) *dst++ = val;
}

struct SegmentDescriptor 
{
  u16 limit0;
  u16 base0;
  u8  base1;
  u8  flags0;
  u8  limit1 : 4;
  u8  flags1 : 4;
  u8  base2;
} __attribute__((packed));

struct TaskStateSegmentDescriptor 
{
  u16 limit0;
  u16 base0;
  u8  base1;
  u8  flags0;
  u8  limit1 : 4;
  u8  flags1 : 4;
  u8  base2;
} __attribute__((packed));

struct NullDescriptor 
{
  u32 hi;
  u32 lo;
};

union Descriptor
{
  struct NullDescriptor null;
  struct SegmentDescriptor segment;
};

struct GDTR
{
  u16 limit;
  u32 base;
} __attribute__((packed));

struct TaskStateSegment 
{
  u32 prev_task_link;       // 0
  u32 esp0;                 // 4
  u32 ss0;                  // 8
  u32 esp1;                 // 12                                                                                         
  u32 ss1;                  // 16                                                                                        
  u32 esp2;                 // 20
  u32 ss2;                  // 24                                                                                        
  u32 cr3;                  // 28                                                                                        
  u32 eip;                  // 32                                                                                        
  u32 eflags;               // 36                                                                                           
  u32 eax;                  // 40                                                                                        
  u32 ecx;                  // 44                                                                                        
  u32 edx;                  // 48                                                                                        
  u32 ebx;                  // 52                                                                                        
  u32 esp;                  // 56                                                                                        
  u32 ebp;                  // 60                                                                                        
  u32 esi;                  // 64                                                                                        
  u32 edi;                  // 68                                                                                        
  u32 es;                   // 72                                                                                       
  u32 cs;                   // 76                                                                                       
  u32 ss;                   // 80                                                                                       
  u32 ds;                   // 84                                                                                       
  u32 fs;                   // 88                                                                                       
  u32 gs;                   // 92                                                                                       
  u32 ldt_segment_selector; // 96
  u16 t : 1;                // 100
  u16 reserved : 15;
  u16 io_map_base;
} __attribute__((packed));

#define PAGE_SIZE           4096
struct TaskStateSegment tss __attribute__((aligned(PAGE_SIZE)));

#define NUM_DESCRIPTORS 6
union Descriptor gdt[NUM_DESCRIPTORS];
struct GDTR gdtr;

#define TSS_STACK_SIZE (1024 / 4)

u32 tss_stack[TSS_STACK_SIZE];

void outb(u16 port, u8 val)
{
  asm volatile ( "outb %0, %1" : : "a"(val), "d"(port) );
}

void remap_pic()
{
  outb(0x20, 0x11);
  outb(0xA0, 0x11);
  outb(0x21, 0x20);
  outb(0xA1, 40);
  outb(0x21, 0x04);
  outb(0xA1, 0x02);
  outb(0x21, 0x01);
  outb(0xA1, 0x01);
  outb(0x21, 0x0);
  outb(0xA1, 0x0);
}

#define PD_NUM_ENTRIES      1024
#define PT_NUM_ENTRIES      1024

#define PAGE_DIR_SIZE       PAGE_SIZE * 1024

#define PDE_PRESENT    ( 1 << 0 )
#define PDE_WRITEABLE  ( 1 << 1 )
#define PDE_USER       ( 1 << 2 )
#define PDE_PWT        ( 1 << 3 ) // page level write through
#define PDE_PCD        ( 1 << 4 ) // page level cache disable
#define PDE_ACCESSED   ( 1 << 5 )
#define PDE_PS         ( 1 << 7 ) // page size, must be 0 for 4096 pages 

#define PTE_PRESENT    ( 1 << 0 )
#define PTE_WRITEABLE  ( 1 << 1 )
#define PTE_USER       ( 1 << 2 )
#define PTE_PWT        ( 1 << 3 ) // page level write through
#define PTE_PCD        ( 1 << 4 ) // page level cache disable
#define PTE_ACCESSED   ( 1 << 5 )
#define PTE_DIRTY      ( 1 << 6 )
#define PTE_PAT        ( 1 << 7 )
#define PTE_GLOBAL     ( 1 << 8 )

typedef u32 PageDirectoryEntry; // PDE
typedef u32 PageTableEntry;     // PTE

// page directory needs to be 4096 byte aligned, because only 20 bits in cr3 are used to address the page directory
PageDirectoryEntry page_dir[PD_NUM_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
// page table needs to be 4096 byte aligned, because only 20 bits in page directory entry are used to address the page table
PageTableEntry     page_table_entries[PD_NUM_ENTRIES * PT_NUM_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

u32 round_down(u32 val, u32 low)
{
  return (val / low) * low;
}

void enable_paging()
{
  asm("push %eax");
  asm("mov %cr0, %eax");
  asm("or $0x80000000, %eax"); // set bit 31 of cr0 to enable paging
  asm("mov %eax, %cr0");
  asm("pop %eax");
}

// cr3 bits 31:12 hold the physical address of the page directory
void write_cr3(u32 val)
{
  asm volatile ("mov %0, %%cr3" :: "a" ( val ) );
}

u32 read_cr3()
{
  u32 cr3 = 0;
  asm volatile ("mov %%cr3, %0" : "=a" ( cr3 ) );
  return cr3;
}

void print_u32(u32 val)
{
  char buffer[10];
  i32 i = 0;

  while(val > 0) 
  {
    buffer[i++] = '0' + (char)(val % 10);
    val /= 10;
  }

  buffer[i++] = '0' + (char)(val % 10);

  while (i--)
  {
    put_char(buffer[i]);
  }
    
  put_char('\n');
}

void print_u32_hex(u32 val)
{
  for(int i = 28; i >=0; i -= 4)
  {
  u8 c = (val >> i) & 0xf;
  switch(c)
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  put_char(c + '0');
  break;
  case 10: put_char('a'); break;
  case 11: put_char('b'); break;
  case 12: put_char('c'); break;
  case 13: put_char('d'); break;
  case 14: put_char('e'); break;
  case 15: put_char('f'); break;
  }
  }
  put_char('\n');
}

void init_gdt()
{
  gdt[0].null.hi = 0;
  gdt[0].null.lo = 0;

  // priviliged code 0x8
  gdt[1].segment.limit0 = 0xffff;
  gdt[1].segment.limit1 = 0xf;

  gdt[1].segment.base0 = 0x0;
  gdt[1].segment.base1 = 0x0;
  gdt[1].segment.base2 = 0x0;

  gdt[1].segment.flags0 = 0x9a;
  gdt[1].segment.flags1 = 0xc;

  // priviliged data 0x10
  gdt[2].segment.limit0 = 0xffff;
  gdt[2].segment.limit1 = 0xf;

  gdt[2].segment.base0 = 0x0;
  gdt[2].segment.base1 = 0x0;
  gdt[2].segment.base2 = 0x0;

  gdt[2].segment.flags0 = 0x92;
  gdt[2].segment.flags1 = 0xc;

  // user code 0x18
  gdt[3].segment.limit0 = 0xffff;
  gdt[3].segment.limit1 = 0xf;

  gdt[3].segment.base0 = 0x0;
  gdt[3].segment.base1 = 0x0;
  gdt[3].segment.base2 = 0x0;

  gdt[3].segment.flags0 = 0xfa;
  gdt[3].segment.flags1 = 0xc;

  // user data 0x20
  gdt[4].segment.limit0 = 0xffff;
  gdt[4].segment.limit1 = 0xf;

  gdt[4].segment.base0 = 0x0;
  gdt[4].segment.base1 = 0x0;
  gdt[4].segment.base2 = 0x0;

  gdt[4].segment.flags0 = 0xf2;
  gdt[4].segment.flags1 = 0xc;

  // tss 0x28
  gdt[5].segment.limit0 = ((sizeof(struct TaskStateSegment) - 1) >>  0) & 0xffff;
  gdt[5].segment.limit1 = ((sizeof(struct TaskStateSegment) - 1) >> 16) & 0x000f;

  gdt[5].segment.base0 = ((u32)(&tss) >>  0) & 0xffff;
  gdt[5].segment.base1 = ((u32)(&tss) >> 16) & 0x000f;
  gdt[5].segment.base2 = ((u32)(&tss) >> 24) & 0x000f;

  gdt[5].segment.flags0 = 0x89;
  gdt[5].segment.flags1 = 0x0;

  // gdtr
  gdtr.limit = sizeof(union Descriptor) * NUM_DESCRIPTORS - 1;
  gdtr.base = (u32) gdt;

  // tss
  tss.ss0 = 0x10; // priviliged data segment descriptor selector
  tss.esp0 = (u32) &tss_stack[TSS_STACK_SIZE - 1];
  tss.cr3 = (u32) &page_dir;

  // load gdt, segment registers and task register
  asm volatile ("lgdt (%0);" :: "a"((u32) &gdtr));
  asm volatile ("    \
    push %eax;       \
    mov $0x10, %eax; \
    mov %eax, %ds;   \
    mov %eax, %es;   \
    mov %eax, %fs;   \
    mov %eax, %gs;   \
    mov %eax, %ss;   \
    mov $0x28, %ax;  \
    ltr %ax;         \
    pop %eax;        \
    ");
}

void init_paging()
{
  // set page table for each page direcory entry 
  for(u32 i = 0; i < PD_NUM_ENTRIES; ++i)
  {
    page_dir[i] = (u32) &page_table_entries[i * PT_NUM_ENTRIES];
  }

  u32 to = round_down((u32)&_bss_end, PAGE_SIZE) + PAGE_SIZE; // exclusive

  for(u32 i = 0; i < to; i += PAGE_SIZE)
  {
    u32 pdIdx = (i >> 22) & 0x3ff;
    u32 ptIdx = (i >> 12) & 0x3ff;

    u32 *pde = &page_dir[pdIdx];
    u32 *pt = (u32*) (*pde & 0xfffff000);
    u32 *pte = &pt[ptIdx];

    *pde |= PDE_PRESENT | PTE_WRITEABLE | PTE_USER;
    *pte = i | PTE_PRESENT | PTE_WRITEABLE | PTE_USER;
  }

  write_cr3( (u32) &page_dir );
  enable_paging();
}

struct InterruptDescriptor
{
  u16 offset0;
  u16 selector;
  u16 flags;
  u16 offset1;
};

// interrupt descriptor table register
struct IDTR
{
  u16 limit;
  u32 base;
} __attribute__((packed));

#define NUM_INTERRUPT_DESCRIPTORS 256
struct InterruptDescriptor interrupt_descriptor_table[NUM_INTERRUPT_DESCRIPTORS];
struct IDTR idtr;

struct ir_frame;

__attribute__((interrupt)) void default_interrupt_handler(struct ir_frame* f)
{
  print("Default interrupt handler.\n");
  while(1);
}
__attribute__((interrupt)) void ir0(struct ir_frame* f)
{
  print("ir0\n");
  while(1);
}
__attribute__((interrupt)) void ir1(struct ir_frame* f)
{
  print("ir1\n");
  while(1);
}
__attribute__((interrupt)) void ir2(struct ir_frame* f)
{
  print("ir2\n");
  while(1);
}
__attribute__((interrupt)) void ir3(struct ir_frame* f)
{
  print("ir3\n");
  while(1);
}
__attribute__((interrupt)) void ir4(struct ir_frame* f)
{
  print("ir4\n");
  while(1);
}
__attribute__((interrupt)) void ir5(struct ir_frame* f)
{
  print("ir5\n");
  while(1);
}
__attribute__((interrupt)) void ir6(struct ir_frame* f)
{
  print("ir6: invalid opcode exception\n");
  while(1);
}
__attribute__((interrupt)) void ir7(struct ir_frame* f)
{
  print("ir7\n");
  while(1);
}
__attribute__((interrupt)) void ir8(struct ir_frame* f)
{
  print("ir8\n");
  while(1);
}
__attribute__((interrupt)) void ir9(struct ir_frame* f)
{
  print("ir9\n");
  while(1);
}
__attribute__((interrupt)) void ir10(struct ir_frame* f)
{
  print("ir10\n");
  while(1);
}
__attribute__((interrupt)) void ir11(struct ir_frame* f)
{
  print("ir11: invalid TSS exception\n");
  while(1);
}
__attribute__((interrupt)) void ir12(struct ir_frame* f)
{
  print("ir12\n");
  while(1);
}
__attribute__((interrupt)) void ir13(struct ir_frame* f)
{
  print("ir13: general protection fault\n");

  u32 volatile error = 0;
  asm volatile ("mov 0x4(%%ebp), %0" : "=r"(error) );
  print("error code:");
  print_u32_hex(error);

  print("ext:");
  print_u32((error >> 0) & 0x1);
  print("idt:");
  print_u32((error >> 1) & 0x1);
  print("ti:");
  print_u32((error >> 2) & 0x1);
  print("segment selector index:");
  print_u32(error >> 3);

  u32 volatile eip = 0;
  asm volatile ("mov 0x8(%%ebp), %0" : "=r"(eip) );
  print("eip:");
  print_u32_hex(eip);

  while(1);

  // TODO: pop error code off the stack
}
__attribute__((interrupt)) void ir14(struct ir_frame* f)
{
  print("ir14: page fault\n");

  u32 volatile error = 0;
  asm volatile ("mov 0x4(%%ebp), %0" : "=r"(error) );
  print("error code:");
  print_u32_hex(error);

  print("p:");
  print_u32_hex((error >> 0) & 0x1);
  print("w/r:");
  print_u32_hex((error >> 1) & 0x1);
  print("u/s:");
  print_u32_hex((error >> 2) & 0x1);
  print("rsvd:");
  print_u32_hex((error >> 3) & 0x1);
  print("i/d:");
  print_u32_hex((error >> 4) & 0x1);
  print("pk:");
  print_u32_hex((error >> 5) & 0x1);
  print("ss:");
  print_u32_hex((error >> 6) & 0x1);
  print("sgx:");
  print_u32_hex((error >> 15) & 0x1);

  u32 volatile eip = 0;
  asm volatile ("mov 0x8(%%ebp), %0" : "=r"(eip) );
  print("eip:");
  print_u32_hex(eip);
  print_u32_hex((u32)user_mode);

  while(1);

  // TODO: pop error code off the stack
}
__attribute__((interrupt)) void ir15(struct ir_frame* f)
{
  print("ir15\n");
  while(1);
}
__attribute__((interrupt)) void ir16(struct ir_frame* f)
{
  print("ir16\n");
  while(1);
}
__attribute__((interrupt)) void ir17(struct ir_frame* f)
{
  print("ir17\n");
  while(1);
}
__attribute__((interrupt)) void ir18(struct ir_frame* f)
{
  print("ir18\n");
  while(1);
}
__attribute__((interrupt)) void ir19(struct ir_frame* f)
{
  print("ir19\n");
  while(1);
}
__attribute__((interrupt)) void ir20(struct ir_frame* f)
{
  print("ir20\n");
  while(1);
}
__attribute__((interrupt)) void ir21(struct ir_frame* f)
{
  print("ir21\n");
  while(1);
}

__attribute__((interrupt)) void timer_interrupt_handler(struct ir_frame* f)
{
  print("Timer tick\n");
  asm("push %eax");
  asm("mov $0x20, %al");
  asm("outb %al, $0x20"); // end of interrupt pic1, expected by the pic master
  asm("outb %al, $0xa0"); // end of interrupt pic2, for pic slave, not always needed
  asm("pop %eax");
}

void enable_interrupts(u32 pIDTR)
{
  asm volatile ("lidt (%0); sti;" :: "a"(pIDTR));
}

void set_interrupt_handler(u8 idx, u32 handler)
{
    struct InterruptDescriptor* id = &interrupt_descriptor_table[idx];

    id->selector = 0x0008; // code segment
    id->offset0  = 0x0000ffff & handler;
    id->offset1  = 0xffff0000 & handler;
    id->flags    = 0x8e00; // type of descriptor (32 bit interrupt call gate)
}

void init_interrupt_handlers()
{
  remap_pic(); // pic master: 32-39, pic slave: 40-47 

  for(int i = 0; i < NUM_INTERRUPT_DESCRIPTORS; ++i)
  {
    set_interrupt_handler(i, (u32) default_interrupt_handler);
  }

  set_interrupt_handler(0, (u32) ir0);
  set_interrupt_handler(1, (u32) ir1);
  set_interrupt_handler(2, (u32) ir2);
  set_interrupt_handler(3, (u32) ir3);
  set_interrupt_handler(4, (u32) ir4);
  set_interrupt_handler(5, (u32) ir5);
  set_interrupt_handler(6, (u32) ir6);
  set_interrupt_handler(7, (u32) ir7);
  set_interrupt_handler(8, (u32) ir8);
  set_interrupt_handler(9, (u32) ir9);
  set_interrupt_handler(10, (u32) ir10);
  set_interrupt_handler(11, (u32) ir11);
  set_interrupt_handler(12, (u32) ir12);
  set_interrupt_handler(13, (u32) ir13);
  set_interrupt_handler(14, (u32) ir14);
  set_interrupt_handler(15, (u32) ir15);
  set_interrupt_handler(16, (u32) ir16);
  set_interrupt_handler(17, (u32) ir17);
  set_interrupt_handler(18, (u32) ir18);
  set_interrupt_handler(19, (u32) ir19);
  set_interrupt_handler(20, (u32) ir20);
  set_interrupt_handler(21, (u32) ir21);

  set_interrupt_handler(32, (u32) timer_interrupt_handler); // the default frequency of the timer interrupts is 18 Hz

  idtr.base = (u32) &interrupt_descriptor_table;
  idtr.limit = sizeof(struct InterruptDescriptor) * NUM_INTERRUPT_DESCRIPTORS - 1;

  enable_interrupts((u32) &idtr);
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
    mov $0x23, %ax; \
    mov %ax, %ds;   \
    mov %ax, %es;   \
    mov %ax, %fs;   \
    mov %ax, %gs;   \
                    \
    mov %esp, %eax; \
                    \
    push $0x23;     \
    push %eax;      \
    pushf;          \
    pop %eax;       \
    or $0x200, %eax;\
    push %eax;      \
    push $0x1b;     \
    push $user_mode;\
    iret;           \
  ");
}

char* base = (char*) (0xb8000);
int current_row = 0;
int current_col = 0;

#define NUM_ROWS 24
#define NUM_COLS 80

void put_char(char c) 
{
  if(c == '\n')
  {
    current_col = 0;
    ++current_row;
    if(current_row >= NUM_ROWS)
    {
      clear_screen();
      current_row = 0;
    }
    return;
  }

  *(base + (current_row * NUM_COLS + current_col) * 2 + 0) = c;
  *(base + (current_row * NUM_COLS + current_col) * 2 + 1) = 128;

  ++current_col;

  if(current_col >= 80) 
  {
    current_col = 0;
    ++current_row;
  }
}

void print(char const* s) 
{
  while(*s != 0)
  {
    put_char(*s);
    ++s;
  }
}

void clear_screen()
{
  char* ptr = (char*) (0xb8000);

  for(int r = 0; r < NUM_ROWS; ++r) 
  {
    for (int c = 0; c < NUM_COLS; ++c) 
    {
      *ptr++ = 0;
      *ptr++ = 0;
    }
  }
}

