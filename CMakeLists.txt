#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.5)
project(MAINUI)

set(CMAKE_CXX_STANDARD 11)

option(MAINUI_USE_CUSTOM_FONT_RENDER "Use custom font rendering" ON)
option(MAINUI_USE_STB "Use stb_truetype.h for rendering(*nix-only)" OFF)
option(MAINUI_FONT_SCALE "Scale fonts by height" OFF)

if(NOT XASH_SDK)
	set(XASH_SDK "sdk_includes/")
endif()

file(GLOB MAINUI_CONTROLS_SOURCES "controls/*.cpp")
file(GLOB MAINUI_MENUS_SOURCES "menus/*.cpp" "menus/dynamic/*.cpp")
file(GLOB MAINUI_FONT_RENDER_SOURCES "font/*.cpp")
file(GLOB MAINUI_VGUI1_SOURCES "vgui1/src/*.cpp")
file(GLOB MAINUI_SOURCES "miniutl/*.cpp" "*.cpp")

add_library(menu SHARED ${MAINUI_CONTROLS_SOURCES} ${MAINUI_MENUS_SOURCES} ${MAINUI_FONT_RENDER_SOURCES} ${MAINUI_VGUI1_SOURCES} ${MAINUI_SOURCES})

if(NOT WIN32 AND NOT MINGW)
	add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable)
endif()

if(MAINUI_FONT_SCALE)
	add_definitions(-DMAINUI_FONT_SCALE)
endif()

add_definitions(-DSTDINT_H=<cstdint>)

# Android historically used stb_truetype because Android NDK ships no
# system FreeType. We now vendor FreeType under 3rdparty/freetype and
# default Android to it - stb_truetype lacks support for native
# TrueType hints (the hand-crafted bytecode Microsoft put in Tahoma)
# and that absence is exactly what made the canon CS 1.6 menu render
# look 'thin/soft' on device. Re-enable stb explicitly with
# -DMAINUI_USE_STB=ON if needed (e.g. for a build that does NOT have
# the FreeType submodule populated).

include_directories(${XASH_SDK}/common ${XASH_SDK}/engine ${XASH_SDK}/pm_shared ${XASH_SDK}/public . controls/ menus/ miniutl/ font/ model/ vgui1/include/)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Font Rendering(FreeType or WinAPI)
if(MAINUI_USE_CUSTOM_FONT_RENDER)
	# Win32 will always use GDI font renderer
	if(NOT WIN32)
		if(MAINUI_USE_STB)
			# Use stbtt
			add_definitions(-DMAINUI_USE_STB)
		else()
			# FreeType. Prefer system FreeType via pkg-config (the way
			# every desktop build has worked for years). When pkg-config
			# fails - mostly Android NDK, where there is no system
			# FreeType - fall back to the vendored 3rdparty/freetype
			# submodule. The vendored build disables every optional
			# dependency (zlib/png/bzip2/harfbuzz/brotli) so the static
			# lib has zero external linkage and slots cleanly into the
			# Android cross-compile.
			find_package(PkgConfig QUIET)
			set(FT2_FOUND OFF)
			if(PKG_CONFIG_FOUND)
				pkg_check_modules(FT2 freetype2)
			endif()
			if(FT2_FOUND)
				include_directories(${FT2_INCLUDE_DIRS})
				target_link_libraries(menu ${FT2_LIBRARIES})
			elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/freetype/CMakeLists.txt")
				message(STATUS "mainui: using vendored FreeType from 3rdparty/freetype")
				set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
				set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
				set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
				set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
				set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
				# libmenu is a SHARED object; the static FreeType inside
				# it must be -fPIC or the linker rejects it on x86_64.
				# Setting CMAKE_POSITION_INDEPENDENT_CODE before the
				# add_subdirectory makes FreeType's own targets pick up
				# the flag.
				set(CMAKE_POSITION_INDEPENDENT_CODE ON)
				add_subdirectory(3rdparty/freetype EXCLUDE_FROM_ALL)
				target_link_libraries(menu freetype)
			else()
				message(FATAL_ERROR "mainui: FreeType not found and 3rdparty/freetype submodule is empty")
			endif()
			add_definitions(-DMAINUI_USE_FREETYPE)
		endif()
	endif()

	add_definitions(-DMAINUI_USE_CUSTOM_FONT_RENDER)
endif()

if(NOT BUILD_AS_PART_OF_ENGINE)
	# grab platform library suffix
	set_target_postfix(menu)
else()
	# install to the same directory as engine
        set(GAMEDIR ".")
        set(CLIENT_INSTALL_DIR ".")

	# On Android, engine expects library named libmenu_android_<arch>.so
	if(ANDROID)
		if(ANDROID_ABI STREQUAL "arm64-v8a")
			set(_MENU_ARCH "arm64")
		elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
			set(_MENU_ARCH "armv7hf")
		elseif(ANDROID_ABI STREQUAL "x86")
			set(_MENU_ARCH "i386")
		elseif(ANDROID_ABI STREQUAL "x86_64")
			set(_MENU_ARCH "amd64")
		else()
			set(_MENU_ARCH "unknown")
		endif()
		set_target_properties(menu PROPERTIES OUTPUT_NAME "menu_android_${_MENU_ARCH}")
	endif()
endif()

install(TARGETS menu DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}")

if(MSVC)
	install(FILES $<TARGET_PDB_FILE:menu> DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}" OPTIONAL)
endif()
