add_executable(kfind)

target_sources(kfind PRIVATE kfinddlg.cpp
    kfinddlg.h
    kfindtreeview.cpp
    kfindtreeview.h
    kftabdlg.cpp
    kftabdlg.h
    kquery.cpp
    kquery.h
    main.cpp)

ecm_qt_declare_logging_category(kfind HEADER kfind_debug.h IDENTIFIER
	KFING_LOG CATEGORY_NAME org.kde.kfind DESCRIPTION "KFind" EXPORT KFIND)


file(GLOB ICONS_SRCS "../icons/*-apps-kfind.png")
ecm_add_app_icon(kfind_SRCS ICONS ${ICONS_SRCS})


target_link_libraries(kfind
Qt${QT_MAJOR_VERSION}::Concurrent
Qt${QT_MAJOR_VERSION}::Widgets
KF5::Archive
KF5::CoreAddons
KF5::FileMetaData
KF5::I18n
KF5::KIOWidgets
KF5::WidgetsAddons
KF5::XmlGui
)

install(TARGETS kfind ${KF_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS org.kde.kfind.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kfind.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

