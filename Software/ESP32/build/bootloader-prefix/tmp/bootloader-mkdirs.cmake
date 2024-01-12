# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/allan/esp/esp-idf/esp-idf/components/bootloader/subproject"
  "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader"
  "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix"
  "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix/tmp"
  "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix/src"
  "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/allan/Documents/freETarget/Software/ESP32/freeETarget/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
