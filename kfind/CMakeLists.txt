cmake_minimum_required(VERSION 2.8.12)

project(Kfind)

# ECM setup
find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
set(KFIND_VERSION "2.0.0")

ecm_setup_version(${KFIND_VERSION} VARIABLE_PREFIX KFIND
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kfind_version.h"
)

# Build dependencies
find_package(KF5 REQUIRED COMPONENTS KDELibs4Support Archive DocTools)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${Kfind_SOURCE_DIR}")
  find_package(KF5Konq CONFIG REQUIRED)
endif()

remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)

set(kfind_SRCS main.cpp
               kfinddlg.cpp
               kftabdlg.cpp
               kquery.cpp
               kdatecombo.cpp
               kfindtreeview.cpp)

file(GLOB ICONS_SRCS "icons/*-apps-kfind.png")
ecm_add_app_icon(kfind_SRCS ICONS ${ICONS_SRCS})

add_executable(kfind ${kfind_SRCS})

target_link_libraries(kfind
KF5::Konq
KF5::Archive
KF5::KDELibs4Support
)

install(TARGETS kfind ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS org.kde.kfind.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kfind.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

add_subdirectory(icons)
add_subdirectory(docs)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${Kfind_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
