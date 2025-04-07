cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "04")
set (RELEASE_SERVICE_VERSION_MICRO "0")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kfind VERSION ${RELEASE_SERVICE_VERSION})

set (KF_MIN_VERSION "6.3.0")
# ECM setup
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(QT_REQUIRED_VERSION "6.7.0")
include(KDEInstallDirs)
find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Concurrent)
find_package(Qt6Core5Compat REQUIRED)

include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)

set(KFIND_VERSION ${RELEASE_SERVICE_VERSION})

ecm_setup_version(${KFIND_VERSION} VARIABLE_PREFIX KFIND
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/kfind_version.h"
)

# Build dependencies
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    CoreAddons
    FileMetaData
    I18n
    KIO
    WidgetsAddons
    XmlGui
    Crash
)
find_package(KF6DocTools ${KF_MIN_VERSION})
set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
ecm_set_disabled_deprecation_versions(QT 6.9.0
    KF 6.12.0
    )
add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
add_subdirectory(src)
add_subdirectory(icons)

ecm_qt_install_logging_categories(EXPORT KFIND FILE kfind.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ki18n_install(po)

if (KF6DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
