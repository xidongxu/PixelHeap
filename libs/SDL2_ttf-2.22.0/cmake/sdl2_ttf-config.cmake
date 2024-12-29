# SDL2_image CMake configuration file:
# This file is meant to be placed in a cmake subfolder of SDL2_image-devel-2.x.y-VC

include(FeatureSummary)
set_package_properties(SDL2_ttf PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_ttf/"
    DESCRIPTION "Support for TrueType (.ttf) font files with Simple Directmedia Layer"
)

cmake_minimum_required(VERSION 3.0...3.28)

set(SDL2_ttf_FOUND TRUE)

set(SDL2TTF_VENDORED TRUE)

set(SDL2TTF_HARFBUZZ TRUE)
set(SDL2TTF_FREETYPE TRUE)

if(CMAKE_SIZEOF_VOID_P STREQUAL "4")
    set(_sdl_arch_subdir "x86")
elseif(CMAKE_SIZEOF_VOID_P STREQUAL "8")
    set(_sdl_arch_subdir "x64")
else()
    unset(_sdl_arch_subdir)
    set(SDL2_ttf_FOUND FALSE)
    return()
endif()

set(_sdl2ttf_incdir       "${CMAKE_CURRENT_LIST_DIR}/../include")
set(_sdl2ttf_library      "${CMAKE_CURRENT_LIST_DIR}/../lib/${_sdl_arch_subdir}/SDL2_ttf.lib")
set(_sdl2ttf_dll          "${CMAKE_CURRENT_LIST_DIR}/../lib/${_sdl_arch_subdir}/SDL2_ttf.dll")

set(SDL2_TTF_INCLUDE_DIRS           "${_sdl2ttf_incdir}")
set_and_check(SDL2_TTF_PREFIX       "${CMAKE_CURRENT_LIST_DIR}/..")
set_and_check(SDL2_TTF_BINDIR       "${SDL2_TTF_PREFIX}/lib/${_sdl_arch_subdir}")
set_and_check(SDL2_TTF_LIBDIR       "${SDL2_TTF_PREFIX}/lib/${_sdl_arch_subdir}")

# All targets are created, even when some might not be requested though COMPONENTS.
# This is done for compatibility with CMake generated SDL2_image-target.cmake files.

if(NOT TARGET SDL2_ttf::SDL2_ttf)
    add_library(SDL2_ttf::SDL2_ttf SHARED IMPORTED)
    set_target_properties(SDL2_ttf::SDL2_ttf
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl2ttf_incdir}"
            IMPORTED_IMPLIB "${_sdl2ttf_library}"
            IMPORTED_LOCATION "${_sdl2ttf_dll}"
            COMPATIBLE_INTERFACE_BOOL "SDL2_SHARED"
            INTERFACE_SDL2_SHARED "ON"
    )
endif()

unset(_sdl_arch_subdir)
unset(_sdl2ttf_incdir)
unset(_sdl2ttf_library)
unset(_sdl2ttf_dll)
