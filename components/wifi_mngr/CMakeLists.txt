set(WEB_EMBED)

include(${CMAKE_CURRENT_LIST_DIR}/../util/scripts/embed_web_files.cmake)

file(GLOB wifi_prov_src
    ${CMAKE_CURRENT_LIST_DIR}/web/wifi_prov.html
)

embed_web_files(wifi_prov_src WEB_EMBED)

idf_component_register(SRCS "src/wifi.c" "src/dns_server.c"
                                        INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "priv_include"
                    EMBED_FILES ${WEB_EMBED}
                    REQUIRES util
                             esp_wifi esp_netif esp_http_server
                    PRIV_REQUIRES nvs_flash)
