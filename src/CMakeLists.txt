
set(kfind_SRCS main.cpp
               kfinddlg.cpp
               kftabdlg.cpp
               kquery.cpp
               kfindtreeview.cpp)

ecm_qt_declare_logging_category(kfind_SRCS HEADER kfind_debug.h IDENTIFIER
               KFING_LOG CATEGORY_NAME org.kde.kfind)


file(GLOB ICONS_SRCS "../icons/*-apps-kfind.png")
ecm_add_app_icon(kfind_SRCS ICONS ${ICONS_SRCS})

add_executable(kfind ${kfind_SRCS})

target_link_libraries(kfind
Qt5::Concurrent
Qt5::Widgets
KF5::Archive
KF5::CoreAddons
KF5::FileMetaData
KF5::I18n
KF5::KIOWidgets
KF5::TextWidgets
KF5::WidgetsAddons
KF5::XmlGui
)

install(TARGETS kfind ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS org.kde.kfind.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kfind.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

