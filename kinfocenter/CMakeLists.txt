add_subdirectory( about ) 

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

add_subdirectory( info )
add_subdirectory( nics )
add_subdirectory( usbview )

macro_optional_find_package(RAW1394)
macro_log_feature(RAW1394_FOUND "RAW1394" "library for direct access to IEEE 1394 bus" "http://www.linux1394.org/" FALSE "" "View FireWire devices in kinfocenter.")

if(RAW1394_FOUND)
   add_subdirectory( view1394 )
endif(RAW1394_FOUND)

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

install(TARGETS kdeinit_kinfocenter  DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( kinfocenter kdeinit_kinfocenter )
install(TARGETS kinfocenter DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES kinfocenter.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kinfocenterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kinfocenter )
