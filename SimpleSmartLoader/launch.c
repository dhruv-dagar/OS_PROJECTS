#include "loader.h"
Elf32_Ehdr *elfHeader;
void *file;


int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }
  int flag=1;
  int f = open(argv[1], O_RDONLY);
  if (f == -1)
  {
    printf("Error: ELF file not opened properly");
    flag= 0;
  }
  int size = lseek(f, 0, SEEK_END);
  lseek(f, 0, SEEK_SET);
  file = malloc(size);
  if (file == NULL)
  {
    printf("Error: Memory allocation failed");
    close(f);
    flag= 0;
  }
  read(f, file, size);
  elfHeader = (Elf32_Ehdr *)file;
  if (elfHeader->e_ident[0] != ELFMAG0 ||
      elfHeader->e_ident[1] != ELFMAG1 ||
      elfHeader->e_ident[2] != ELFMAG2 ||
      elfHeader->e_ident[3] != ELFMAG3)
  {
    printf("Not a valid ELF file.\n");
    free(file);
    close(f);
    flag = 0;
  }
  free(file);
  close(f);
  if(flag==1){
    load_and_run_elf(argv[1]);
    loader_cleanup();
  }
  return 0;
}
