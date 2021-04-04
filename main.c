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

i32 trigger_interrupt();

void start() 
{
  clear_screen();

  print("Init interrupts...\n");
  init_interrupt_handlers();
  print("Interrupts initialized!\n");

  print("Init paging...\n");
  init_paging();
  print("Paging initialized!\n");
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
  print("Default interrupt handler.");
  while(1);
}

void load_idt(u32 pIDTR)
{
  asm("mov 8(%esp), %eax");
  asm("lidt (%eax)");
}

void init_interrupt_handlers()
{
  for(int i = 0; i < NUM_INTERRUPT_DESCRIPTORS; ++i)
  {
    struct InterruptDescriptor* id = &interrupt_descriptor_table[i];
    u32 ir = (u32) default_interrupt_handler;

    id->selector = 0x0008;
    id->offset0  = 0x0000ffff & ir;
    id->offset1  = 0xffff0000 & ir;
    id->flags    = 0x8e00;
  }

  idtr.base = (u32) &interrupt_descriptor_table;
  idtr.limit = sizeof(struct InterruptDescriptor) * NUM_INTERRUPT_DESCRIPTORS - 1;

  load_idt((u32) &idtr);
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

