cmake_minimum_required(VERSION 3.14)

project(cspacefs-uefi)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("-O3")

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:efi_main")
    add_compile_options("/GS-")
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

    # work around bug in Visual Studio
    if (${MSVC_CXX_ARCHITECTURE_ID} STREQUAL "X86")
        set(CMAKE_SYSTEM_PROCESSOR "X86")
    endif()
else()
    add_compile_options(-fno-stack-check -fno-stack-protector -mno-stack-arg-probe)
endif()

include_directories(/usr/include/efi)
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    include_directories(/usr/include/efi/x86_64)
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "X86")
    include_directories(/usr/include/efi/ia32)
endif()

set(SRC_FILES src/cspacefs.cpp)

add_executable(cspacefs ${SRC_FILES})

set_target_properties(cspacefs PROPERTIES SUFFIX ".efi")

if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(cspacefs PRIVATE "-ffreestanding")
    target_compile_options(cspacefs PRIVATE "-fno-stack-protector")
    target_compile_options(cspacefs PRIVATE "-fno-stack-check")
    target_compile_options(cspacefs PRIVATE "-mno-stack-arg-probe")

    target_link_options(cspacefs PRIVATE "-nostartfiles")
    target_link_options(cspacefs PRIVATE "-shared")

    if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        target_link_options(cspacefs PRIVATE "-Wl,--subsystem,efi_boot_service_driver")
    else()
        target_link_options(cspacefs PRIVATE "-Wl,--subsystem,11")
    endif()

    if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "X86")
        target_link_options(cspacefs PRIVATE "-e_efi_main")
    else()
        target_link_options(cspacefs PRIVATE "-eefi_main")
    endif()
elseif(MSVC)
    target_link_options(cspacefs PRIVATE "/SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER")
    target_compile_options(cspacefs PRIVATE "/Oi-")
endif()

target_compile_options(cspacefs PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall -Wextra -Wno-address-of-packed-member -Werror=pointer-arith -fno-exceptions>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4 /Oi->)