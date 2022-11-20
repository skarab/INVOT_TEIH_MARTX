# Install script for directory: D:/prog/intro/music/WaveSabre

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/WaveSabre")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/prog/intro/music/WaveSabre/build/MSVCRT/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/WaveSabreCore/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/WaveSabrePlayerLib/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/Tests/PlayerTest/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/WaveSabreStandAlonePlayer/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/WaveSabreVstLib/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/Vsts/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/WaveSabreConvert/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/Tests/WaveSabreConvertTests/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/ConvertTheFuck/cmake_install.cmake")
  include("D:/prog/intro/music/WaveSabre/build/ProjectManager/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/prog/intro/music/WaveSabre/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
