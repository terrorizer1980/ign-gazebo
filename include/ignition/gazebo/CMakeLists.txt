ign_install_all_headers()

add_subdirectory(components)
add_subdirectory(gui)

install (FILES server.config DESTINATION ${IGN_DATA_INSTALL_DIR})
