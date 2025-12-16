add_executable(server
  main.c
  ftp_utils.c
  client.c
  client_handler.c
  active_client_registry.c
  console_manager.c
  user_table.c
)

target_link_libraries(server PRIVATE 
  pthread
)

