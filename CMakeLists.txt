cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "21")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "80")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kfind VERSION ${RELEASE_SERVICE_VERSION})

set (KF5_MIN_VERSION "5.85.0")
# ECM setup
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(QT_REQUIRED_VERSION "5.15.2")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Concurrent)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)

set(KFIND_VERSION ${RELEASE_SERVICE_VERSION})

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
    WidgetsAddons
)
add_definitions(
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050e00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055500
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(doc)

ecm_qt_install_logging_categories(EXPORT KFIND FILE kfind.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
