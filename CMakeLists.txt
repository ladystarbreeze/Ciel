cmake_minimum_required(VERSION 3.15)
project(Ciel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -O3")

find_package(SDL2 REQUIRED)
include_directories(Ciel ${SDL2_INCLUDE_DIRS})

add_executable(Ciel main.cpp src/nes.cpp src/nes.h src/mmu/mmu.cpp src/mmu/mmu.h src/mmu/mappers/mapper_interface/mapper.h src/mmu/mappers/mappers.h src/mmu/mappers/mapper_implementations/nrom.cpp src/mmu/mappers/mapper_implementations/nrom.h src/mmu/cartridge.cpp src/mmu/cartridge.h src/cpu/cpu.cpp src/cpu/cpu.h src/ppu/ppu.cpp src/ppu/ppu.h src/mmu/mappers/mapper_implementations/axrom.cpp src/mmu/mappers/mapper_implementations/axrom.h)
target_link_libraries(Ciel ${SDL2_LIBRARIES})