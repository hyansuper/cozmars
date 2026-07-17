find_package(Python3 REQUIRED)

# embed_web_files(<src_list_var> <embed_var>)
#
# For each source file in the list variable <src_list_var>, adds a custom
# command that minifies+gzips it to <src>.gz and appends the result to the
# list variable <embed_var> in the parent scope (suitable for EMBED_FILES).
function(embed_web_files src_list_var embed_var)
    foreach(src IN LISTS ${src_list_var})
        set(gz ${src}.gz)
        list(APPEND ${embed_var} ${gz})
        if(NOT CMAKE_SCRIPT_MODE_FILE)
            add_custom_command(
                OUTPUT ${gz}
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/minify_gzip.py
                        ${src} ${gz}
                DEPENDS ${src}
                COMMENT "Compressing ${src}"
            )
        endif()
    endforeach()
    set(${embed_var} ${${embed_var}} PARENT_SCOPE)
endfunction()
