file(REMOVE_RECURSE
  "config/sdkconfig.h"
  "config/sdkconfig.cmake"
  "bootloader.bin"
  "bootloader.map"
  "project_elf_src.c"
  "CMakeFiles/erase_flash"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/erase_flash.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
