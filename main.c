void start(); // has to be first symbol

typedef int i32;
typedef short int i16;
typedef char i8;

typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char u8;

void put_char(char);
void print(char const*);
void clear_screen();

void init_paging();
void init_interrupt_handlers();

void switch_to_user_mode();

void print_u32();

void start() 
{
  clear_screen();

  print("Init paging...\n");
  init_paging();
  print("Paging initialized!\n");

  print("Init interrupts...\n");
  init_interrupt_handlers();
  print("Interrupts initialized!\n");

  print("Switching to user mode...\n");
  switch_to_user_mode();

  print("Not reached");
  while(1);
}

void user_mode()
{
  print("User mode.\n");
  while(1); 
}

extern u32 _bss_end;

void memset(u8* dst, u8 val, u32 size)
{
  while(size--) *dst++ = val;
}

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

#define PAGE_SIZE           4096
#define PAGE_DIR_SIZE       PAGE_SIZE * 1024

#define PDE_PRESENT    ( 1 << 0 )
#define PDE_WRITEABLE  ( 1 << 1 )
#define PDE_SUPERVISOR ( 1 << 2 )
#define PDE_PWT        ( 1 << 3 ) // page level write through
#define PDE_PCD        ( 1 << 4 ) // page level cache disable
#define PDE_ACCESSED   ( 1 << 5 )
#define PDE_PS         ( 1 << 7 ) // page size, must be 0 for 4096 pages 

#define PTE_PRESENT    ( 1 << 0 )
#define PTE_WRITEABLE  ( 1 << 1 )
#define PTE_SUPERVISOR ( 1 << 2 )
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
PageTableEntry     page_table_entries[PD_NUM_ENTRIES * PT_NUM_ENTRIES] __attribute__((aligned(PAGE_SIZE)));;

u32 round_down(u32 val, u32 low)
{
  return (val / low) * low;
}

void enable_paging()
{
  asm("mov %cr0, %eax");
  asm("or $0x80000000, %eax"); // set bit 31 of cr0 to enable paging
  asm("mov %eax, %cr0");
}

// cr3 bits 31:12 hold the physical address of the page directory
void set_cr3(u32 val)
{
  asm("mov 8(%esp), %eax");
  asm("mov %eax, %cr3");
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

    *pde |= PDE_PRESENT;
    *pte = i | PTE_PRESENT | PTE_WRITEABLE;
  }

  set_cr3( (u32) &page_dir );
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
}
__attribute__((interrupt)) void ir0(struct ir_frame* f)
{
  print("ir0\n");
}
__attribute__((interrupt)) void ir1(struct ir_frame* f)
{
  print("ir1\n");
}
__attribute__((interrupt)) void ir2(struct ir_frame* f)
{
  print("ir2\n");
}
__attribute__((interrupt)) void ir3(struct ir_frame* f)
{
  print("ir3\n");
}
__attribute__((interrupt)) void ir4(struct ir_frame* f)
{
  print("ir4\n");
}
__attribute__((interrupt)) void ir5(struct ir_frame* f)
{
  print("ir5\n");
}
__attribute__((interrupt)) void ir6(struct ir_frame* f)
{
  print("ir6\n");
}
__attribute__((interrupt)) void ir7(struct ir_frame* f)
{
  print("ir7\n");
}
__attribute__((interrupt)) void ir8(struct ir_frame* f)
{
  print("ir8\n");
}
__attribute__((interrupt)) void ir9(struct ir_frame* f)
{
  print("ir9\n");
}
__attribute__((interrupt)) void ir10(struct ir_frame* f)
{
  print("ir10\n");
}
__attribute__((interrupt)) void ir11(struct ir_frame* f)
{
  print("ir11\n");
}
__attribute__((interrupt)) void ir12(struct ir_frame* f)
{
  print("ir12\n");
}
__attribute__((interrupt)) void ir13(struct ir_frame* f)
{
  print("ir13\n");
}
__attribute__((interrupt)) void ir14(struct ir_frame* f)
{
  print("ir14\n");
}
__attribute__((interrupt)) void ir15(struct ir_frame* f)
{
  print("ir15\n");
}
__attribute__((interrupt)) void ir16(struct ir_frame* f)
{
  print("ir16\n");
}
__attribute__((interrupt)) void ir17(struct ir_frame* f)
{
  print("ir17\n");
}
__attribute__((interrupt)) void ir18(struct ir_frame* f)
{
  print("ir18\n");
}
__attribute__((interrupt)) void ir19(struct ir_frame* f)
{
  print("ir19\n");
}
__attribute__((interrupt)) void ir20(struct ir_frame* f)
{
  print("ir20\n");
}
__attribute__((interrupt)) void ir21(struct ir_frame* f)
{
  print("ir21\n");
}

__attribute__((interrupt)) void timer_interrupt_handler(struct ir_frame* f)
{
  asm("mov $0x20, %al");
  asm("outb %al, $0x20"); // end of interrupt pic1, expected by the pic master
  asm("outb %al, $0xa0"); // end of interrupt pic2, for pic slave, not always needed
}

void enable_interrupts(u32 pIDTR)
{
  asm("mov 8(%esp), %eax");
  asm("lidt (%eax)");
  asm("sti");
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
    mov $0x10, %ax; \
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
    pushl $0x08;    \
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

