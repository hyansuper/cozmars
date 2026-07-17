file(GLOB server_web_src
    ${CMAKE_CURRENT_LIST_DIR}/camera.html
)

embed_web_files(server_web_src WEB_EMBED)
