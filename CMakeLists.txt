#
#  Copyright 2025 e-soul.org
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without modification, are permitted
#  provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice, this list of conditions
#     and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
#     and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

cmake_minimum_required(VERSION 3.21)
project(GenericOverlay)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(GenericOverlay main.cc)

if(WIN32)
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)
    target_link_libraries(GenericOverlay PRIVATE user32 SDL3::SDL3 SDL3_ttf::SDL3_ttf)
    set_target_properties(GenericOverlay PROPERTIES WIN32_EXECUTABLE ON)
    add_custom_command(TARGET GenericOverlay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:GenericOverlay> $<TARGET_FILE_DIR:GenericOverlay>
        COMMAND_EXPAND_LISTS
    )
else()
    find_package(X11 REQUIRED)
    find_library(XFIXES_LIBRARY Xfixes REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    pkg_check_modules(SDL3_ttf REQUIRED IMPORTED_TARGET sdl3-ttf)

    target_link_libraries(GenericOverlay PRIVATE ${X11_LIBRARIES} ${XFIXES_LIBRARY} PkgConfig::SDL3 PkgConfig::SDL3_ttf)
endif()
