set(WEB_EMBED)

include(${CMAKE_CURRENT_LIST_DIR}/../../../components/util/scripts/embed_web_files.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/server/CMakeLists.txt)

idf_component_register(SRCS "main.c" "server/camera_server.c"
                    INCLUDE_DIRS "." "server"
                    EMBED_FILES ${WEB_EMBED}
                    REQUIRES cozmars wifi_mngr util
                             esp_new_jpeg
                    PRIV_REQUIRES nvs_flash esp_http_server)
