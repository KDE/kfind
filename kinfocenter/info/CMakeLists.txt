# TODO: HAVE_COREAUDIO (for OSX)
# TODO: HAVE_LIBDEVINFO_H (for Solaris 7 and later)
#   to be set if both -ldevinfo and libdevinfo.h exist

check_include_files(devinfo.h HAVE_DEVINFO_H)       # info_fbsd.cpp
check_include_files(fstab.h HAVE_FSTAB_H)           # info_linux.cpp
check_include_files(linux/raw.h HAVE_LINUX_RAW_H)   # info_linux.cpp
check_include_files(mntent.h HAVE_MNTENT_H)         # info_linux.cpp
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)   # info_linux.cpp
check_include_files(sys/raw.h HAVE_SYS_RAW_H)       # info_linux.cpp
check_include_files(Alib.h HAVE_ALIB_H)             # info_hpux.cpp

configure_file (config-infocenter.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-infocenter.h )

if(OPENGL_FOUND AND OPENGL_GLU_FOUND)
   ADD_DEFINITIONS(-DINFO_OPENGL_AVAILABLE ) # added manually
   include_directories( ${OPENGL_INCLUDE_DIR} ${OPENGL_GLU_INCLUDE_DIR} )
else(OPENGL_FOUND AND OPENGL_GLU_FOUND)
   MESSAGE(STATUS "OpenGL information module has been disabled.")
endif(OPENGL_FOUND AND OPENGL_GLU_FOUND)
########### next target ###############

set(kcm_info_PART_SRCS main.cpp memory.cpp opengl.cpp )


kde4_add_plugin(kcm_info ${kcm_info_PART_SRCS})


target_link_libraries(kcm_info  ${KDE4_KDEUI_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QT3SUPPORT_LIBRARY} ${X11_X11_LIB})
if(OPENGL_FOUND AND OPENGL_GLU_FOUND)
  target_link_libraries(kcm_info ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
endif(OPENGL_FOUND AND OPENGL_GLU_FOUND)

install(TARGETS kcm_info  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(FILES memory.desktop
	# processor.desktop # currently broken
	dma.desktop interrupts.desktop ioports.desktop opengl.desktop pci.desktop sound.desktop devices.desktop scsi.desktop partitions.desktop xserver.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

