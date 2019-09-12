cmake_minimum_required(VERSION 3.0)

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "19")
set (KDE_APPLICATIONS_VERSION_MINOR "08")
set (KDE_APPLICATIONS_VERSION_MICRO "1")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(kfind VERSION ${KDE_APPLICATIONS_VERSION})

set (KF5_MIN_VERSION "5.28.0")
# ECM setup
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(QT_REQUIRED_VERSION "5.9.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Concurrent)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)

set(KFIND_VERSION ${KDE_APPLICATIONS_VERSION})

ecm_setup_version(${KFIND_VERSION} VARIABLE_PREFIX KFIND
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/kfind_version.h"
)

# Build dependencies
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    CoreAddons
    DocTools
    FileMetaData
    I18n
    KIO
    TextWidgets
    WidgetsAddons
)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(doc)

if (${ECM_VERSION} STRGREATER "5.58.0")
    install(FILES kfind.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES kfind.categories  DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
