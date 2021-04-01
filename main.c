
void start(); // has to be first symbol

void put_char(char);
void print(char const*);
void clear_screen();

void start() 
{
  clear_screen();
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
  print("Hello!\n");
}

char* base = (char*) (0xb8000);
int currentRow = 0;
int currentCol = 0;
int const nrows = 24;
int const ncols = 80;


void put_char(char c) {
    *(base + (currentRow * ncols + currentCol) * 2 + 0) = c;
    *(base + (currentRow * ncols + currentCol) * 2 + 1) = 128;

    ++currentCol;

    if(currentCol >= 80) {
	    currentCol = 0;
	    ++currentRow;
    }
}

void print(char const* s) {
   while(*s != 0) {
     if(*s == '\n') {
	currentCol = 0;
	++currentRow;
	++s;
	continue;
     }
     put_char(*s);
     ++s;
   }
}

void clear_screen() {
  char* ptr = (char*) (0xb8000);

  for(int r = 0; r < nrows; ++r) {
	  for (int c = 0; c < ncols; ++c) {
		  *ptr++ = 0;
		  *ptr++ = 0;
	}
  }

}

