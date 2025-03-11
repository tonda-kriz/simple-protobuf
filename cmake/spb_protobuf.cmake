include(CMakeParseArguments)

if(SPB_PROTO_USE_CLANG_FORMAT)
    find_program(CLANG_FORMAT clang-format)
endif()

function(spb_protobuf_generate SRCS HDRS)
    if (NOT SPB_PROTO_OUTPUT_DIR)
        set(SPB_PROTO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    else()
        # Convert to absolute path
        cmake_path(ABSOLUTE_PATH SPB_PROTO_OUTPUT_DIR 
                BASE_DIRECTORY ${CMAKE_SOURCE_DIR}
                NORMALIZE
                OUTPUT_VARIABLE SPB_PROTO_OUTPUT_DIR)
        file(MAKE_DIRECTORY ${SPB_PROTO_OUTPUT_DIR})
    endif ()
    
    if (NOT ARGN)
        message(FATAL_ERROR "Error: spb_protobuf_generate() called without any proto files")
        return()
    endif ()
    
    set(${SRCS})
    set(${HDRS})
    
    foreach (FIL ${ARGN})
        get_filename_component(FILE_NAME ${FIL} NAME_WE)
        get_filename_component(FILE_ABS ${FIL} ABSOLUTE)
        
        if(TARGET spb-proto)
            # add directory with generated files to the spb-proto includes
            target_include_directories(spb-proto INTERFACE ${SPB_PROTO_OUTPUT_DIR})
        endif()
        
        list(APPEND ${SRCS} "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.cc")
        list(APPEND ${HDRS} "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.h")

        file(MAKE_DIRECTORY ${SPB_PROTO_OUTPUT_DIR})

        if(SPB_PROTO_USE_CLANG_FORMAT AND CLANG_FORMAT)
            add_custom_command(
                OUTPUT "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.cc"
                       "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.h"
                COMMAND $<TARGET_FILE:spb-protoc> ARGS "--cpp_out=${SPB_PROTO_OUTPUT_DIR}" ${FILE_ABS}
                COMMAND ${CLANG_FORMAT} ARGS -i "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.cc"
                COMMAND ${CLANG_FORMAT} ARGS -i "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.h"
                DEPENDS spb-protoc ${FILE_ABS}
                COMMENT "Compiling protofile ${FIL}"
                VERBATIM
            )
        else()
            add_custom_command(
                OUTPUT "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.cc"
                       "${SPB_PROTO_OUTPUT_DIR}/${FILE_NAME}.pb.h"
                COMMAND $<TARGET_FILE:spb-protoc> ARGS "--cpp_out=${SPB_PROTO_OUTPUT_DIR}" ${FILE_ABS}
                DEPENDS spb-protoc ${FILE_ABS}
                COMMENT "Compiling protofile ${FIL}"
                VERBATIM
            )
        endif()
    endforeach ()

    set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
    set(${SRCS} ${${SRCS}} PARENT_SCOPE)
    set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction(spb_protobuf_generate)
