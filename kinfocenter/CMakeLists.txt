add_subdirectory( about ) 

add_definitions (-DQT3_SUPPORT)

set(kinfocenter_SRCS
   main.cpp 
   toplevel.cpp 
   indexwidget.cpp 
   helpwidget.cpp 
   dockcontainer.cpp 
   aboutwidget.cpp 
   quickhelp.cpp 
   moduletreeview.cpp 
   moduleiconview.cpp 
   moduleIface.cpp 
   global.cpp 
   modules.cpp 
   proxywidget.cpp 
   kcrootonly.cpp 
   searchwidget.cpp )

set(kinfocenter_adaptor ${KDEBASE_APPS_SOURCE_DIR}/kinfocenter/moduleIface.h)
set(kinfocenter_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Kinfocenter.xml)
ADD_CUSTOM_COMMAND(OUTPUT ${kinfocenter_xml}
       COMMAND ${QT_DBUSCPP2XML_EXECUTABLE} ${kinfocenter_adaptor} > ${kinfocenter_xml}
       DEPENDS ${kinfocenter_adaptor})
QT4_ADD_DBUS_INTERFACE( kinfocenter_SRCS ${kinfocenter_xml} kinfocenter_interface )

set(kinfocenter_KDEINIT_SRCS ${kinfocenter_SRCS})

kde4_add_kdeinit_executable( kinfocenter ${kinfocenter_KDEINIT_SRCS})

target_link_libraries(kdeinit_kinfocenter  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KUTILS_LIBS} )

install(TARGETS kdeinit_kinfocenter  DESTINATION ${LIB_INSTALL_DIR} )

target_link_libraries( kinfocenter kdeinit_kinfocenter )
install(TARGETS kinfocenter DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES kinfocenter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kinfocenterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kinfocenter )

kde4_install_icons( ${ICON_INSTALL_DIR}  )
