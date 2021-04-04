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

void print_u32();

u32 timer_cnt = 0;

void start() 
{
  clear_screen();

  print("Init paging...\n");
  init_paging();
  print("Paging initialized!\n");

  print("Init interrupts...\n");
  init_interrupt_handlers();
  print("Interrupts initialized!\n");

  while(1)
  {
    if(timer_cnt >= 18)
    {
      print("Tick\n");
      timer_cnt = 0;
    }
  }
}

extern u32 _bss_end;

void memset(u8* dst, u8 val, u32 size)
{
  while(size--) *dst++ = val;
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

__attribute__((interrupt)) void timer_interrupt_handler(struct ir_frame* f)
{
  ++timer_cnt;
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
  for(int i = 0; i < NUM_INTERRUPT_DESCRIPTORS; ++i)
  {
    set_interrupt_handler(i, (u32) default_interrupt_handler);
  }

  set_interrupt_handler(8, (u32) timer_interrupt_handler); // by default, channel 0 of the pic is linked to interrupt vector 8, this should be reconfigured, because it conflicts with the x86 errors / interrupts; the default frequency of the timer interrupts is 18 Hz

  idtr.base = (u32) &interrupt_descriptor_table;
  idtr.limit = sizeof(struct InterruptDescriptor) * NUM_INTERRUPT_DESCRIPTORS - 1;

  enable_interrupts((u32) &idtr);
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

