
OPTION(RUN_KAPPFINDER "Installing non-KDE applications" OFF)

add_subdirectory( apps )
add_subdirectory( pics )


add_definitions (-DQT3_SUPPORT)

########### next target ###############

set(kappfinder_SRCS main.cpp toplevel.cpp common.cpp )

kde4_automoc(${kappfinder_SRCS})

kde4_add_executable(kappfinder ${kappfinder_SRCS})

target_link_libraries(kappfinder  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS kappfinder DESTINATION ${BIN_INSTALL_DIR})


########### next target ###############

set(kappfinder_install_SRCS main_install.cpp common.cpp )

kde4_automoc(${kappfinder_install_SRCS})

kde4_add_executable(kappfinder_install ${kappfinder_install_SRCS})

target_link_libraries(kappfinder_install  ${KDE4_KDEUI_LIBS} ${QT_QT3SUPPORT_LIBRARY} )


########### install files ###############

install( FILES kappfinder.desktop  DESTINATION  ${XDG_APPS_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  )

if(RUN_KAPPFINDER)
   file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_kappfinder.cmake "exec_program(${CMAKE_CURRENT_BINARY_DIR}/kappfinder_install ARGS \"${DATA_INSTALL_DIR}\")\n")
   # and add it as post-install script to any of the installed targets, so it will be executed during "make install"
   set_target_properties(kappfinder_install PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/run_kappfinder.cmake)
endif(RUN_KAPPFINDER)

