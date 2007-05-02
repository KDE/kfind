
add_subdirectory( Arcade ) 
add_subdirectory( Board ) 
add_subdirectory( Card ) 
add_subdirectory( Emulators ) 
add_subdirectory( TacticStrategy ) 
add_subdirectory( Roguelikes ) 



########### install files ###############

install( FILES penguin-command.desktop xshipwars.desktop  	Clanbomber.desktop defendguin.desktop GTali.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kappfinder/apps/Games )
