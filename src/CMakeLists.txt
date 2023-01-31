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
KF${KF_MAJOR_VERSION}::Archive
KF${KF_MAJOR_VERSION}::CoreAddons
KF${KF_MAJOR_VERSION}::FileMetaData
KF${KF_MAJOR_VERSION}::I18n
KF${KF_MAJOR_VERSION}::KIOWidgets
KF${KF_MAJOR_VERSION}::WidgetsAddons
KF${KF_MAJOR_VERSION}::XmlGui
)
if (TARGET Qt::Core5Compat)
    target_link_libraries(kfind Qt::Core5Compat)
endif()

install(TARGETS kfind ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS org.kde.kfind.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kfind.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

