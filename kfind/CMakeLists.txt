set(kfind_SRCS main.cpp
               kfinddlg.cpp
               kftabdlg.cpp
               kquery.cpp
               kdatecombo.cpp
               kfindtreeview.cpp)

kde4_add_app_icon(kfind_SRCS "hi*-app-kfind.png")

add_executable(kfind ${kfind_SRCS})

target_link_libraries(kfind konq KF5::Archive)

install(TARGETS kfind ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( PROGRAMS kfind.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

ecm_install_icons( ${ICON_INSTALL_DIR}  )
