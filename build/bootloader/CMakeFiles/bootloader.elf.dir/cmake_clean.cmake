file(REMOVE_RECURSE
  "config/sdkconfig.h"
  "config/sdkconfig.cmake"
  "bootloader.bin"
  "bootloader.map"
  "project_elf_src.c"
  "project_elf_src.c"
  "CMakeFiles/bootloader.elf.dir/project_elf_src.c.obj"
  "bootloader.elf.pdb"
  "bootloader.elf"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/bootloader.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
