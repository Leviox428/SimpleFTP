add_executable(client
  main.c
  client.c
  ftp_utils.c
)

target_link_libraries(client PRIVATE
  pthread
)
