cmake_minimum_required(VERSION 3.16)

project(moonphase VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

add_library(moonphase_core calculations.cpp)
target_include_directories(moonphase_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(moonphase_core PUBLIC cxx_std_17)

add_executable(moonphase main.cpp)
target_compile_features(moonphase PRIVATE cxx_std_17)
target_link_libraries(moonphase PRIVATE moonphase_core)

install(TARGETS moonphase
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES README.md LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

set(CPACK_PACKAGE_NAME "moonphase")
set(CPACK_PACKAGE_VENDOR "moonphase")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Moon phase and moonrise calculator")
set(CPACK_PACKAGE_CONTACT "moonphase maintainers")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "moonphase")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "moonphase maintainers")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only")
endif()

include(CPack)
