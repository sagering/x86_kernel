void start(); // has to be first symbol

void put_char(char);
void print(char const*);
void clear_screen();

void init_paging();

void start() 
{
  clear_screen();
  print("Initializing paging...\n");
  init_paging();
  print("Paging initialized!\n");
}

typedef unsigned int  u32;
typedef unsigned char u8;

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

// page directory needs to be 4096 byte aligned
PageDirectoryEntry page_dir[PD_NUM_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageTableEntry     page_table_entries[PD_NUM_ENTRIES * PT_NUM_ENTRIES];

u32 round_down(u32 val, u32 low)
{
  return (val / low) * low;
}

void enable_paging()
{
  asm("mov %cr0, %eax");
  asm("or $0x80000000, %eax");
  asm("mov %eax, %cr0");
}

// cr3 bits 31:12 hold the physical address of the page directory
void set_cr3(u32 val)
{
  asm("mov 8(%esp), %eax");
  asm("mov %eax, %cr3");
}

void init_paging()
{
  for(u32 i = 0; i < PD_NUM_ENTRIES; ++i)
  {
    page_dir[i] = (u32) &page_table_entries[i * PT_NUM_ENTRIES];
  }

  u32 from = 0;
  u32 to = round_down((u32)&_bss_end, PAGE_SIZE) + PAGE_SIZE; // exclusive

  for(u32 i = from; i < to; i += PAGE_SIZE)
  {
    u32 pdIdx = (i >> 22) & 0x3ff;
    u32 ptIdx = (i >> 12) & 0x3ff;

    u32 *pde = &page_dir[pdIdx];
    *pde |= PDE_PRESENT;
    u32 *pt = (u32*) (*pde & 0xfffff000);
    u32 *pte = &pt[ptIdx];
    *pte = i | PTE_PRESENT | PTE_WRITEABLE;
  }

  set_cr3( (u32) &page_dir );
  enable_paging( (u32) &page_dir );
}

char* base = (char*) (0xb8000);
int current_row = 0;
int current_col = 0;

#define NUM_ROWS 24
#define NUM_COLS 80

void put_char(char c) 
{
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
    if(*s == '\n')
    {
      current_col = 0;
      ++current_row;
      ++s;
      continue;
    }
    put_char(*s);
    ++s;
  }
}

void clear_screen() {
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

