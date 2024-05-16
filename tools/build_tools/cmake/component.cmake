# include($ENV{OSK_PATH}/component.cmake)
#
# Internal function for retrieving component properties from a component target.
#
function(__component_get_property var component_target property)
    get_property(val TARGET ${component_target} PROPERTY ${property})
    set(${var} "${val}" PARENT_SCOPE)
endfunction()

#
# Internal function for setting component properties on a component target. As with build properties,
# set properties are also keeped track of.
#
function(__component_set_property component_target property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})

    if(__APPEND)
        set_property(TARGET ${component_target} APPEND PROPERTY ${property} "${val}")
    else()
        set_property(TARGET ${component_target} PROPERTY ${property} "${val}")
    endif()

    # Keep track of set component properties
    __component_get_property(properties ${component_target} __COMPONENT_PROPERTIES)
    if(NOT property IN_LIST properties)
        __component_set_property(${component_target} __COMPONENT_PROPERTIES ${property} APPEND)
    endif()
endfunction()

#
# Given a component name or alias, get the corresponding component target.
#
function(__component_get_target var name_or_alias)
    # Look at previously resolved names or aliases
    armino_build_get_property(component_names_resolved __COMPONENT_NAMES_RESOLVED)
    list(FIND component_names_resolved ${name_or_alias} result)
    if(NOT result EQUAL -1)
        # If it has been resolved before, return that value. The index is the same
        # as in __COMPONENT_NAMES_RESOLVED as these are parallel lists.
        armino_build_get_property(component_targets_resolved __COMPONENT_TARGETS_RESOLVED)
        list(GET component_targets_resolved ${result} target)
        set(${var} ${target} PARENT_SCOPE)
        return()
    endif()

    armino_build_get_property(component_targets __COMPONENT_TARGETS)

    # Assume first that the paramters is an alias.
    string(REPLACE "::" "_" name_or_alias "${name_or_alias}")
    set(component_target ___${name_or_alias})

    if(component_target IN_LIST component_targets)
        set(${var} ${component_target} PARENT_SCOPE)
        set(target ${component_target})
    else() # assumption is wrong, try to look for it manually
        unset(target)
        foreach(component_target ${component_targets})
            __component_get_property(_component_name ${component_target} COMPONENT_NAME)
            if(name_or_alias STREQUAL _component_name)
                set(target ${component_target})
                break()
            endif()
        endforeach()
        set(${var} ${target} PARENT_SCOPE)
    endif()

    # Save the resolved name or alias
    if(target)
        armino_build_set_property(__COMPONENT_NAMES_RESOLVED ${name_or_alias} APPEND)
        armino_build_set_property(__COMPONENT_TARGETS_RESOLVED ${target} APPEND)
    endif()
endfunction()

#
# Called during component registration, sets basic properties of the current component.
#
macro(__component_set_properties)
    __component_get_property(type ${component_target} COMPONENT_TYPE)

    # Fill in the rest of component property
    __component_set_property(${component_target} SRCS "${sources}")
    __component_set_property(${component_target} INCLUDE_DIRS "${__INCLUDE_DIRS}")

    if(type STREQUAL LIBRARY)
        __component_set_property(${component_target} PRIV_INCLUDE_DIRS "${__PRIV_INCLUDE_DIRS}")
    endif()

    __component_set_property(${component_target} LDFRAGMENTS "${__LDFRAGMENTS}")
    __component_set_property(${component_target} EMBED_FILES "${__EMBED_FILES}")
    __component_set_property(${component_target} EMBED_TXTFILES "${__EMBED_TXTFILES}")
    __component_set_property(${component_target} REQUIRED_ARMINO_SOCS "${__REQUIRED_ARMINO_SOCS}")
endmacro()

#
# Perform a quick check if given component dir satisfies basic requirements.
#
function(__component_dir_quick_check var component_dir)
    set(res 1)
    get_filename_component(abs_dir ${component_dir} ABSOLUTE)

    # Check this is really a directory and that a CMakeLists.txt file for this component exists
    # - warn and skip anything which isn't valid looking (probably cruft)
    if(NOT IS_DIRECTORY "${abs_dir}")
        set(res 0)
    endif()

    get_filename_component(base_dir ${abs_dir} NAME)
    string(SUBSTRING "${base_dir}" 0 1 first_char)

    if(NOT first_char STREQUAL ".")
        if(NOT EXISTS "${abs_dir}/CMakeLists.txt")
            set(res 0)
        endif()
    else()
        set(res 0) # quietly ignore dot-folders
    endif()

    set(${var} ${res} PARENT_SCOPE)
endfunction()


function(armino_component_register_call_subdirs)
endfunction()

function(__component_loop_sub_dirs var component_dir)
    set(res 0)

    if (EXISTS ${component_dir}/CMakeLists.txt)
        execute_process(COMMAND
            grep -o "armino_component_register_call_subdirs" ${component_dir}/CMakeLists.txt
            OUTPUT_VARIABLE ouput_strings
            RESULT_VARIABLE result
        )

        if ("${ouput_strings}" MATCHES "armino_component_register_call_subdirs")
            set(res 1)
        endif()
    endif()

    set(${var} ${res} PARENT_SCOPE)
endfunction()

#
# Write a CMake file containing all component and their properties. This is possible because each component
# keeps a list of all its properties.
#
function(__component_write_properties output_file)
    armino_build_get_property(component_targets __COMPONENT_TARGETS)
    foreach(component_target ${component_targets})
        __component_get_property(component_properties ${component_target} __COMPONENT_PROPERTIES)
        foreach(property ${component_properties})
            __component_get_property(val ${component_target} ${property})
            set(component_properties_text
                "${component_properties_text}\nset(__component_${component_target}_${property} ${val})")
        endforeach()
        file(WRITE ${output_file} "${component_properties_text}")
    endforeach()
endfunction()

#
# Add a component to process in the build. The components are keeped tracked of in property
# __COMPONENT_TARGETS in component target form.
#
function(__component_add component_dir prefix)
    # For each component, two entities are created: a component target and a component library. The
    # component library is created during component registration (the actual static/interface library).
    # On the other hand, component targets are created early in the build
    # (during adding component as this function suggests).
    # This is so that we still have a target to attach properties to up until the component registration.
    # Plus, interface libraries have limitations on the types of properties that can be set on them,
    # so later in the build, these component targets actually contain the properties meant for the
    # corresponding component library.
    armino_build_get_property(component_targets __COMPONENT_TARGETS)
    get_filename_component(abs_dir ${component_dir} ABSOLUTE)
    get_filename_component(base_dir ${abs_dir} NAME)

    if(NOT EXISTS "${abs_dir}/CMakeLists.txt")
        LOGI("Directory '${component_dir}' does not contain a component.")
    endif()

    armino_build_get_property(inc_prefix __INCLUDE_PREFIX)
    armino_build_get_property(component_prefix __PREFIX)
    # The component target has three underscores as a prefix. The corresponding component library
    # only has two.
    # check the component's name in include dir is same to component dir's
    set(if_inc_component_repeat)
    if(prefix STREQUAL component_prefix)
        set(component_name ${base_dir})
        set(component_target ___${prefix}_${component_name})
    else()
        set(if_repeat_target ___${component_prefix}_${base_dir})
        if(if_repeat_target IN_LIST component_targets)
            set(if_inc_component_repeat 1)
            set(component_name ${prefix}_${base_dir})
            set(component_target ___${component_name})
        else()
            set(component_name ${base_dir})
            set(component_target ___${prefix}_${component_name})
        endif()
    endif()

    # If a component of the same name has been added before, just override the properties.
    # As a side effect, components added later 'override' components added earlier.
    if(NOT component_target IN_LIST component_targets)
        if(NOT TARGET ${component_target})
            add_library(${component_target} STATIC IMPORTED)
        endif()
        armino_build_set_property(__COMPONENT_TARGETS ${component_target} APPEND)
    else()
        if(EXISTS "${abs_dir}/../CMakeLists.txt")
            execute_process(COMMAND
                grep -o "armino_component_register_call_subdirs" ${abs_dir}/../CMakeLists.txt
                OUTPUT_VARIABLE ouput_strings
                RESULT_VARIABLE result
            )
            if ("${ouput_strings}" MATCHES "armino_component_register_call_subdirs")
                execute_process(COMMAND
                    grep -o "^if (.*)" ${abs_dir}/../CMakeLists.txt
                    OUTPUT_VARIABLE ouput_strings
                    RESULT_VARIABLE result
                )
                if (NOT "${ouput_strings}" STREQUAL "")
                    string(LENGTH ${ouput_strings} ouput_len)
                    math(EXPR ouput_len "${ouput_len}-6")
                    string(SUBSTRING ${ouput_strings} 4 ${ouput_len} ouput_substr)
                    if (NOT (${ouput_substr}))
                        return()
                    endif()
                endif()
            endif()
        endif()
        __component_get_property(dir ${component_target} COMPONENT_DIR)
        __component_set_property(${component_target} COMPONENT_OVERRIDEN_DIR ${dir})
    endif()

    set(component_dir ${abs_dir})
    if(prefix STREQUAL component_prefix)
        set(component_lib __${prefix}_${component_name})
        set(component_alias ${prefix}::${component_name}) # The 'alias' of the component library,
                                                    # used to refer to the component outside
                                                    # the build system. Users can use this name
                                                    # to resolve ambiguity with component names
                                                    # and to link ARMINO components to external targets.
    else()
        if(if_inc_component_repeat)
            set(component_lib __${prefix}_${base_dir})
            set(component_alias ${prefix}::${base_dir})
        else()
            set(component_lib __${prefix}_${component_name})
            set(component_alias ${prefix}::${component_name})
        endif()
    endif()
    set(component_base_dir ${base_dir})
    # Set the basic properties of the component
    __component_set_property(${component_target} COMPONENT_LIB ${component_lib})
    __component_set_property(${component_target} COMPONENT_NAME ${component_name})
    __component_set_property(${component_target} COMPONENT_DIR ${component_dir})
    __component_set_property(${component_target} COMPONENT_ALIAS ${component_alias})
    __component_set_property(${component_target} COMPONENT_BASE_DIR ${component_base_dir})

    __component_set_property(${component_target} __PREFIX ${prefix})

    # Set Kconfig related properties on the component
    __kconfig_component_init(${component_target})
endfunction()

#
# Given a component directory, get the requirements by expanding it early. The expansion is performed
# using a separate CMake script (the expansion is performed in a separate instance of CMake in scripting mode).
#
function(__component_get_requirements)
    armino_build_get_property(armino_path ARMINO_PATH)

    armino_build_get_property(build_dir BUILD_DIR)
    set(build_properties_file ${build_dir}/build_properties.temp.cmake)
    set(component_properties_file ${build_dir}/component_properties.temp.cmake)
    set(component_requires_file ${build_dir}/component_requires.temp.cmake)

    __project_reload()
    __build_write_properties(${build_properties_file})
    __component_write_properties(${component_properties_file})

    execute_process(COMMAND "${CMAKE_COMMAND}"
        -D "BEKEN_PLATFORM=1"
        -D "BUILD_PROPERTIES_FILE=${build_properties_file}"
        -D "COMPONENT_PROPERTIES_FILE=${component_properties_file}"
        -D "COMPONENT_REQUIRES_FILE=${component_requires_file}"
        -P "${armino_path}/tools/build_tools/cmake/scripts/component_get_requirements.cmake"
        RESULT_VARIABLE result
        ERROR_VARIABLE error)

    if(NOT result EQUAL 0)
        LOGI("${error}")
    endif()

    armino_build_get_property(armino_component_manager ARMINO_COMPONENT_MANAGER)
    if(armino_component_manager AND armino_component_manager EQUAL "1")
        # Call for component manager once again to inject dependencies
        armino_build_get_property(python PYTHON)
        execute_process(COMMAND ${python}
            "-m"
            "armino_component_manager.prepare_components"
            "--project_dir=${project_dir}"
            "inject_requrements"
            "--armino_path=${armino_path}"
            "--build_dir=${build_dir}"
            "--component_requires_file=${component_requires_file}"
            RESULT_VARIABLE result
            ERROR_VARIABLE error)

        if(NOT result EQUAL 0)
            LOGI("${error}")
        endif()
    endif()

    include(${component_requires_file})

    file(REMOVE ${build_properties_file})
    file(REMOVE ${component_properties_file})
    file(REMOVE ${component_requires_file})
endfunction()

# __component_add_sources, __component_check_target, __component_add_include_dirs
#
# Utility macros for component registration. Adds source files and checks target requirements,
# and adds include directories respectively.
macro(__component_add_sources sources)
    set(sources "")
    if(__SRCS)
        if(__SRC_DIRS)
            LOGI("SRCS and SRC_DIRS are both specified; ignoring SRC_DIRS.")
        endif()
        foreach(src ${__SRCS})
            get_filename_component(src "${src}" ABSOLUTE BASE_DIR ${COMPONENT_DIR})
            list(APPEND sources ${src})
        endforeach()
    else()
        if(__SRC_DIRS)
            foreach(dir ${__SRC_DIRS})
                get_filename_component(abs_dir ${dir} ABSOLUTE BASE_DIR ${COMPONENT_DIR})

                if(NOT IS_DIRECTORY ${abs_dir})
                    LOGI("SRC_DIRS entry '${dir}' does not exist.")
                endif()

                file(GLOB dir_sources "${abs_dir}/*.c" "${abs_dir}/*.cpp" "${abs_dir}/*.S")

                if(dir_sources)
                    foreach(src ${dir_sources})
                        get_filename_component(src "${src}" ABSOLUTE BASE_DIR ${COMPONENT_DIR})
                        list(APPEND sources "${src}")
                    endforeach()
                else()
                    LOGI("No source files found for SRC_DIRS entry '${dir}'.")
                endif()
            endforeach()
        endif()

        if(__EXCLUDE_SRCS)
            foreach(src ${__EXCLUDE_SRCS})
                get_filename_component(src "${src}" ABSOLUTE)
                list(REMOVE_ITEM sources "${src}")
            endforeach()
        endif()
    endif()

    list(REMOVE_DUPLICATES sources)
endmacro()

macro(__component_add_include_dirs lib dirs type)
    foreach(dir ${dirs})
        get_filename_component(_dir ${dir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        if(NOT IS_DIRECTORY ${_dir})
            LOGI("Include directory '${_dir}' is not a directory.")
        endif()
        target_include_directories(${lib} ${type} ${_dir})
    endforeach()
endmacro()

macro(__component_check_target)
    if(__REQUIRED_ARMINO_SOCS)
        armino_build_get_property(armino_target ARMINO_SOC)
        if(NOT armino_target IN_LIST __REQUIRED_ARMINO_SOCS)
            LOGI("Component ${COMPONENT_NAME} only supports targets: ${__REQUIRED_ARMINO_SOCS}")
        endif()
    endif()
endmacro()

# __component_set_dependencies, __component_set_all_dependencies
#
#  Links public and private requirements for the currently processed component
macro(__component_set_dependencies reqs type)
    foreach(req ${reqs})
        if(req IN_LIST build_component_targets)
            __component_get_property(req_lib ${req} COMPONENT_LIB)
            if("${type}" STREQUAL "PRIVATE")
                set_property(TARGET ${component_lib} APPEND PROPERTY LINK_LIBRARIES ${req_lib})
                set_property(TARGET ${component_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES $<LINK_ONLY:${req_lib}>)
            elseif("${type}" STREQUAL "PUBLIC")
                set_property(TARGET ${component_lib} APPEND PROPERTY LINK_LIBRARIES ${req_lib})
                set_property(TARGET ${component_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${req_lib})
            else() # INTERFACE
                set_property(TARGET ${component_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${req_lib})
            endif()
        endif()
    endforeach()
endmacro()

macro(__component_set_all_dependencies)
    __component_get_property(type ${component_target} COMPONENT_TYPE)
    armino_build_get_property(build_component_targets __BUILD_COMPONENT_TARGETS)

    if(NOT type STREQUAL CONFIG_ONLY)
        __component_get_property(reqs ${component_target} __REQUIRES)
        __component_set_dependencies("${reqs}" PUBLIC)

        __component_get_property(priv_reqs ${component_target} __PRIV_REQUIRES)

        #ONLY USED PRIV_REQUIRES, __PRIV_REQUIRES is not true
        __component_get_property(priv_reqs_1 ${component_target} PRIV_REQUIRES)
        __component_set_property(${component_target} __PRIV_REQUIRES "")
        foreach(req ${priv_reqs_1})
            __component_get_target(priv_reqs_0 ${req})
            __component_set_property(${component_target} __PRIV_REQUIRES ${priv_reqs_0} APPEND)
        endforeach()
        __component_get_property(priv_reqs ${component_target} __PRIV_REQUIRES)
        __component_set_dependencies("${priv_reqs}" PRIVATE)

    else()
        __component_get_property(reqs ${component_target} __REQUIRES)
        __component_set_dependencies("${reqs}" INTERFACE)
    endif()
endmacro()


# armino_component_get_property
#
# @brief Retrieve the value of the specified component property
#
# @param[out] var the variable to store the value of the property in
# @param[in] component the component name or alias to get the value of the property of
# @param[in] property the property to get the value of
#
# @param[in, optional] GENERATOR_EXPRESSION (option) retrieve the generator expression for the property
#                   instead of actual value
function(armino_component_get_property var component property)
    cmake_parse_arguments(_ "GENERATOR_EXPRESSION" "" "" ${ARGN})
    __component_get_target(component_target ${component})
    if(__GENERATOR_EXPRESSION)
        set(val "$<TARGET_PROPERTY:${component_target},${property}>")
    else()
        __component_get_property(val ${component_target} ${property})
    endif()
    set(${var} "${val}" PARENT_SCOPE)
endfunction()

# armino_component_set_property
#
# @brief Set the value of the specified component property related. The property is
#        also added to the internal list of component properties if it isn't there already.
#
# @param[in] component component name or alias of the component to set the property of
# @param[in] property the property to set the value of
# @param[out] value value of the property to set to
#
# @param[in, optional] APPEND (option) append the value to the current value of the
#                     property instead of replacing it
function(armino_component_set_property component property val)
    cmake_parse_arguments(_ "APPEND" "" "" ${ARGN})
    __component_get_target(component_target ${component})

    if(__APPEND)
        __component_set_property(${component_target} ${property} "${val}" APPEND)
    else()
        __component_set_property(${component_target} ${property} "${val}")
    endif()
endfunction()


# armino_component_register
#
# @brief Register a component to the build, creating component library targets etc.
#
# @param[in, optional] SRCS (multivalue) list of source files for the component
# @param[in, optional] SRC_DIRS (multivalue) list of source directories to look for source files
#                       in (.c, .cpp. .S); ignored when SRCS is specified.
# @param[in, optional] EXCLUDE_SRCS (multivalue) used to exclude source files for the specified
#                       SRC_DIRS
# @param[in, optional] INCLUDE_DIRS (multivalue) public include directories for the created component library
# @param[in, optional] PRIV_INCLUDE_DIRS (multivalue) private include directories for the created component library
# @param[in, optional] LDFRAGMENTS (multivalue) linker script fragments for the component
# @param[in, optional] REQUIRES (multivalue) publicly required components in terms of usage requirements
# @param[in, optional] PRIV_REQUIRES (multivalue) privately required components in terms of usage requirements
#                      or components only needed for functions/values defined in its project_include.cmake
# @param[in, optional] REQUIRED_ARMINO_SOCS (multivalue) the list of ARMINO build targets that the component only supports
# @param[in, optional] EMBED_FILES (multivalue) list of binary files to embed with the component
# @param[in, optional] EMBED_TXTFILES (multivalue) list of text files to embed with the component
# @param[in, optional] KCONFIG (single value) override the default Kconfig
# @param[in, optional] KCONFIG_PROJBUILD (single value) override the default Kconfig
# @param[in, optional] INTERNAL_LIB (single value) override the default Kconfig
function(armino_component_register)
    set(options INTERNAL_LIB)
    set(single_value KCONFIG KCONFIG_PROJBUILD)
    set(multi_value SRCS SRC_DIRS EXCLUDE_SRCS
                    INCLUDE_DIRS PRIV_INCLUDE_DIRS LDFRAGMENTS REQUIRES
                    PRIV_REQUIRES REQUIRED_ARMINO_SOCS EMBED_FILES EMBED_TXTFILES)
    cmake_parse_arguments(_ "${options}" "${single_value}" "${multi_value}" ${ARGN})

    if(NOT __armino_component_context)
        LOGI("Called armino_component_register from a non-component directory.")
    endif()

    __component_check_target()
    __component_add_sources(sources)

    redefine_file_macro("${sources}")
    
    # Add component manifest and lock files to list of dependencies
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPONENT_DIR}/armino_component.yml")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPONENT_DIR}/dependencies.lock")

    # Create the final target for the component. This target is the target that is
    # visible outside the build system.
    __component_get_target(component_target ${COMPONENT_ALIAS})
    __component_get_property(component_lib ${component_target} COMPONENT_LIB)
    __component_get_property(component_base_dir ${component_target} COMPONENT_BASE_DIR)
    __component_get_property(component_alias ${component_target} COMPONENT_ALIAS)
    __component_get_property(component_name ${component_target} COMPONENT_NAME)
    __component_get_property(prefix ${component_target} __PREFIX)
    # armino_build_get_property(component_targets __COMPONENT_TARGETS)

    # LOGI("[armino_component_register]: component_name is ${component_name}.")
    # LOGI("[armino_component_register]: component_target is ${component_target}.")

    armino_build_get_property(component_targets __BUILD_COMPONENT_TARGETS)
    armino_build_get_property(component_libs __BUILD_COMPONENTS)
    armino_build_get_property(component_aliass BUILD_COMPONENT_ALIASES)
    armino_build_get_property(component_names BUILD_COMPONENTS)
    armino_build_get_property(inc_prefix __INCLUDE_PREFIX)
    armino_build_get_property(component_prefix __PREFIX)

    # LOGI("[armino_component_register]: component_targets is ${component_targets}.")

    set(if_inc_component_repeat)
    set(repeat_target)
    if(prefix STREQUAL inc_prefix)
        # set(repeat_target ___${component_prefix}_${component_base_dir})
        string(REPLACE "${inc_prefix}" "${component_prefix}" repeat_target "${component_alias}")
        string(REPLACE "::" "_" repeat_target "${repeat_target}")
        set(repeat_target ___${repeat_target})
        if(repeat_target IN_LIST component_targets)
            set(if_inc_component_repeat 1)
        endif()
    endif()

    # Use generator expression so that users can append/override flags even after call to
    # armino_build_process
    armino_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
    armino_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
    armino_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    armino_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    armino_build_get_property(common_reqs ___COMPONENT_REQUIRES_COMMON)

    include_directories("${include_directories}")
    add_compile_options("${compile_options}")
    add_c_compile_options("${c_compile_options}")
    add_cxx_compile_options("${cxx_compile_options}")

    # Unfortunately add_definitions() does not support generator expressions. A new command
    # add_compile_definition() does but is only available on CMake 3.12 or newer. This uses
    # add_compile_options(), which can add any option as the workaround.
    #
    # TODO: Use add_compile_definitions() once minimum supported version is 3.12 or newer.
    armino_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
    add_compile_options("${compile_definitions}")

    list(REMOVE_ITEM common_reqs ${component_lib})
    link_libraries(${common_reqs})

    armino_build_get_property(config_dir CONFIG_DIR)

    # The contents of 'sources' is from the __component_add_sources call
    if(sources OR __EMBED_FILES OR __EMBED_TXTFILES)
        add_library(${component_lib} STATIC ${sources})
        __component_set_property(${component_target} COMPONENT_TYPE LIBRARY)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" PUBLIC)
        __component_add_include_dirs(${component_lib} "${__PRIV_INCLUDE_DIRS}" PRIVATE)
        __component_add_include_dirs(${component_lib} "${config_dir}" PUBLIC)
        set_target_properties(${component_lib} PROPERTIES OUTPUT_NAME ${COMPONENT_NAME})
        __ldgen_add_component(${component_lib})
    else()
        if(if_inc_component_repeat AND repeat_target)
            __component_get_property(repeat_lib ${repeat_target} COMPONENT_LIB)
            __component_add_include_dirs(${repeat_lib} "${__INCLUDE_DIRS}" INTERFACE)
            __component_add_include_dirs(${repeat_lib} "${config_dir}" INTERFACE)
            list(REMOVE_ITEM component_targets ${component_target})
            list(REMOVE_ITEM component_libs ${component_lib})
            list(REMOVE_ITEM component_aliass ${component_alias})
            list(REMOVE_ITEM component_names ${component_name})
            armino_build_set_property(__BUILD_COMPONENT_TARGETS "${component_targets}")
            armino_build_set_property(__BUILD_COMPONENTS "${component_libs}")
            armino_build_set_property(BUILD_COMPONENT_ALIASES "${component_aliass}")
            armino_build_set_property(BUILD_COMPONENTS "${component_names}")
            armino_build_get_property(component_aliass BUILD_COMPONENT_ALIASES)
            return()
        else()
        add_library(${component_lib} INTERFACE)
        __component_set_property(${component_target} COMPONENT_TYPE CONFIG_ONLY)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" INTERFACE)
        __component_add_include_dirs(${component_lib} "${config_dir}" INTERFACE)
        endif()
    endif()

    # Alias the static/interface library created for linking to external targets.
    # The alias is the <prefix>::<component name> name.
    __component_get_property(component_alias ${component_target} COMPONENT_ALIAS)
    add_library(${component_alias} ALIAS ${component_lib})

    # Perform other component processing, such as embedding binaries and processing linker
    # script fragments
    foreach(file ${__EMBED_FILES})
        target_add_binary_data(${component_lib} "${file}" "BINARY")
    endforeach()

    foreach(file ${__EMBED_TXTFILES})
        target_add_binary_data(${component_lib} "${file}" "TEXT")
    endforeach()

    if(__LDFRAGMENTS)
        __ldgen_add_fragment_files("${__LDFRAGMENTS}")
    endif()

    # Set dependencies
    __component_set_all_dependencies()

    # Make the COMPONENT_LIB variable available in the component CMakeLists.txt
    set(COMPONENT_LIB ${component_lib} PARENT_SCOPE)
    # COMPONENT_TARGET is deprecated but is made available with same function
    # as COMPONENT_LIB for compatibility.
    set(COMPONENT_TARGET ${component_lib} PARENT_SCOPE)

    __component_set_properties()
endfunction()

# armino_component_register_ex
#
# @brief Register a component(can set component name) to the build, creating component library targets etc.
#
# @param[in, optional] SRCS (multivalue) list of source files for the component
# @param[in, optional] SRC_DIRS (multivalue) list of source directories to look for source files
#                       in (.c, .cpp. .S); ignored when SRCS is specified.
# @param[in, optional] EXCLUDE_SRCS (multivalue) used to exclude source files for the specified
#                       SRC_DIRS
# @param[in, optional] INCLUDE_DIRS (multivalue) public include directories for the created component library
# @param[in, optional] PRIV_INCLUDE_DIRS (multivalue) private include directories for the created component library
# @param[in, optional] LDFRAGMENTS (multivalue) linker script fragments for the component
# @param[in, optional] REQUIRES (multivalue) publicly required components in terms of usage requirements
# @param[in, optional] PRIV_REQUIRES (multivalue) privately required components in terms of usage requirements
#                      or components only needed for functions/values defined in its project_include.cmake
# @param[in, optional] REQUIRED_ARMINO_SOCS (multivalue) the list of ARMINO build targets that the component only supports
# @param[in, optional] KCONFIG (single value) override the default Kconfig
# @param[in, optional] KCONFIG_PROJBUILD (single value) override the default Kconfig
# @param[in, optional] INTERNAL_LIB (single value)
# @param[in, optional] CONFIG_COMPONENT_NAME (single value) set customized component name
function(armino_component_register_ex)
    set(options INTERNAL_LIB)
    set(single_value KCONFIG KCONFIG_PROJBUILD CONFIG_COMPONENT_NAME)
    set(multi_value SRCS SRC_DIRS EXCLUDE_SRCS
                    INCLUDE_DIRS PRIV_INCLUDE_DIRS LDFRAGMENTS REQUIRES
                    PRIV_REQUIRES REQUIRED_ARMINO_SOCS)
    cmake_parse_arguments(_ "${options}" "${single_value}" "${multi_value}" ${ARGN})

    if(NOT __armino_component_context)
        LOGI("Called armino_component_register from a non-component directory.")
    endif()

    LOGI("[armino_component_register_ex]: customized component name is ${__CONFIG_COMPONENT_NAME}.")

    __component_check_target()
    __component_add_sources(sources)

    redefine_file_macro("${sources}")

    # Add component manifest and lock files to list of dependencies
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPONENT_DIR}/armino_component.yml")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMPONENT_DIR}/dependencies.lock")

    armino_build_get_property(prefix __PREFIX)



    ################################################################################################
    # Set component name begin
    ################################################################################################
    armino_build_get_property(component_targets __COMPONENT_TARGETS)

    get_filename_component(abs_dir ${COMPONENT_DIR} ABSOLUTE)
    get_filename_component(base_dir ${abs_dir} NAME)

    set(config_component_name ${__CONFIG_COMPONENT_NAME})
    # LOGI("[__component_add_ex]: config_component_name is ${config_component_name}.")
    # LOGI("[__component_add_ex]: base_dir is ${base_dir}.")

    armino_build_get_property(inc_prefix __INCLUDE_PREFIX)
    armino_build_get_property(component_prefix __PREFIX)

    # LOGI("[__component_add_ex]: inc_prefix is ${inc_prefix}.")
    # LOGI("[__component_add_ex]: component_prefix is ${component_prefix}.")

    # The component target has three underscores as a prefix.
    # The corresponding component library only has two.
    set(component_name  ${prefix}_${config_component_name})
    set(component_alias ${prefix}::${config_component_name})
    set(component_lib     __${component_name})
    set(component_target ___${component_name})
    set(component_dir      ${abs_dir})
    set(component_base_dir ${base_dir})

    # LOGI("[__component_add_ex]: component_name is ${component_name}.")
    # LOGI("[__component_add_ex]: component_target is ${component_target}.")


    if(NOT ${base_dir} STREQUAL ${config_component_name})

        # If a component of the same name has been added before, just override the properties.
        # As a side effect, components added later 'override' components added earlier.
        if(NOT component_target IN_LIST component_targets)
            if(NOT TARGET ${component_target})
                # LOGI("[__component_add_ex]: add_library ${component_target}")
                add_library(${component_target} STATIC IMPORTED)
            endif()
            armino_build_set_property(__COMPONENT_TARGETS ${component_target} APPEND)
        else()
            # LOGI("[__component_add_ex]: ${component_target} already registered.")
            return()
        endif()

        # Set the basic properties of the component
        __component_set_property(${component_target} COMPONENT_LIB ${component_lib})
        __component_set_property(${component_target} COMPONENT_NAME ${component_name})
        __component_set_property(${component_target} COMPONENT_DIR ${component_dir})
        __component_set_property(${component_target} COMPONENT_ALIAS ${component_alias})
        __component_set_property(${component_target} COMPONENT_BASE_DIR ${component_base_dir})

        # LOGI("[__component_add_ex]: component_alias is ${component_alias}.")
        __component_set_property(${component_target} __PREFIX ${prefix})

        # Set Kconfig related properties on the component
        __kconfig_component_init(${component_target})
        # LOGI("[__component_add_ex]: ${component_target} added ok.")
    else()
        # LOGI("[__component_add_ex]: ${component_target} already added.")
    endif() 
    ################################################################################################
    # Set component name end
    ################################################################################################

    # Create the final target for the component. This target is the target that is
    # visible outside the build system.

    armino_build_get_property(component_targets __BUILD_COMPONENT_TARGETS)
    armino_build_get_property(component_libs __BUILD_COMPONENTS)
    armino_build_get_property(component_aliass BUILD_COMPONENT_ALIASES)
    armino_build_get_property(component_names BUILD_COMPONENTS)
    armino_build_get_property(inc_prefix __INCLUDE_PREFIX)
    armino_build_get_property(component_prefix __PREFIX)

    # LOGI("[armino_component_register_ex]: component_targets is ${component_targets}.")
    # LOGI("[armino_component_register_ex]: component_libs is ${component_libs}.")
    # LOGI("[armino_component_register_ex]: component_aliass is ${component_aliass}.")
    # LOGI("[armino_component_register_ex]: component_names is ${component_names}.")
    # LOGI("[armino_component_register_ex]: inc_prefix is ${inc_prefix}.")
    # LOGI("[armino_component_register_ex]: component_prefix is ${component_prefix}.")

    set(if_inc_component_repeat)
    set(repeat_target)
    if(prefix STREQUAL inc_prefix)
        # set(repeat_target ___${component_prefix}_${component_base_dir})
        string(REPLACE "${inc_prefix}" "${component_prefix}" repeat_target "${component_alias}")
        string(REPLACE "::" "_" repeat_target "${repeat_target}")
        set(repeat_target ___${repeat_target})
        if(repeat_target IN_LIST component_targets)
            set(if_inc_component_repeat 1)
        endif()
    endif()

    # Use generator expression so that users can append/override flags even after call to
    # armino_build_process
    armino_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
    armino_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
    armino_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    armino_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    armino_build_get_property(common_reqs ___COMPONENT_REQUIRES_COMMON)

    include_directories("${include_directories}")
    add_compile_options("${compile_options}")
    add_c_compile_options("${c_compile_options}")
    add_cxx_compile_options("${cxx_compile_options}")

    # Unfortunately add_definitions() does not support generator expressions. A new command
    # add_compile_definition() does but is only available on CMake 3.12 or newer. This uses
    # add_compile_options(), which can add any option as the workaround.
    #
    # TODO: Use add_compile_definitions() once minimum supported version is 3.12 or newer.
    armino_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
    add_compile_options("${compile_definitions}")

    # LOGI("[armino_component_register_ex]: common_reqs is ${common_reqs}.")
    list(REMOVE_ITEM common_reqs ${component_lib})
    link_libraries(${common_reqs})

    armino_build_get_property(config_dir CONFIG_DIR)

    # The contents of 'sources' is from the __component_add_sources call
    if(sources)
        add_library(${component_lib} STATIC ${sources})
        __component_set_property(${component_lib} COMPONENT_TYPE LIBRARY)
        __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" PUBLIC)
        __component_add_include_dirs(${component_lib} "${__PRIV_INCLUDE_DIRS}" PRIVATE)
        __component_add_include_dirs(${component_lib} "${config_dir}" PUBLIC)
        set_target_properties(${component_lib} PROPERTIES OUTPUT_NAME ${config_component_name})
        __ldgen_add_component(${component_lib})
    else()
        if(if_inc_component_repeat AND repeat_target)
            __component_get_property(repeat_lib ${repeat_target} COMPONENT_LIB)
            __component_add_include_dirs(${repeat_lib} "${__INCLUDE_DIRS}" INTERFACE)
            __component_add_include_dirs(${repeat_lib} "${config_dir}" INTERFACE)
            list(REMOVE_ITEM component_targets ${component_target})
            list(REMOVE_ITEM component_libs ${component_lib})
            list(REMOVE_ITEM component_aliass ${component_alias})
            list(REMOVE_ITEM component_names ${component_name})
            armino_build_set_property(__BUILD_COMPONENT_TARGETS "${component_targets}")
            armino_build_set_property(__BUILD_COMPONENTS "${component_libs}")
            armino_build_set_property(BUILD_COMPONENT_ALIASES "${component_aliass}")
            armino_build_set_property(BUILD_COMPONENTS "${component_names}")
            armino_build_get_property(component_aliass BUILD_COMPONENT_ALIASES)
            return()
        else()
            add_library(${component_lib} INTERFACE)
            __component_set_property(${component_target} COMPONENT_TYPE CONFIG_ONLY)
            __component_add_include_dirs(${component_lib} "${__INCLUDE_DIRS}" INTERFACE)
            __component_add_include_dirs(${component_lib} "${config_dir}" INTERFACE)
        endif()
    endif()

    # Alias the static/interface library created for linking to external targets.
    # The alias is the <prefix>::<component name> name.
    __component_get_property(component_alias ${component_target} COMPONENT_ALIAS)
    add_library(${component_alias} ALIAS ${component_lib})

    if(__LDFRAGMENTS)
        __ldgen_add_fragment_files("${__LDFRAGMENTS}")
    endif()


    # LOGI("[armino_component_register_ex]: __REQUIRES: ${__REQUIRES}, __PRIV_REQUIRES: ${__PRIV_REQUIRES}.")
    __component_set_property(${component_target} REQUIRES "${__REQUIRES}")
    __component_set_property(${component_target} PRIV_REQUIRES "${__PRIV_REQUIRES}")

    # Set dependencies
    __component_set_all_dependencies()

    set(__component_registered 1)
    __component_set_property(${component_target} COMPONENT_REGISTERED ${__component_registered})
    __build_expand_requirements(${component_target})


    # Make the COMPONENT_LIB variable available in the component CMakeLists.txt
    set(COMPONENT_LIB ${component_lib} PARENT_SCOPE)
    # COMPONENT_TARGET is deprecated but is made available with same function
    # as COMPONENT_LIB for compatibility.
    set(COMPONENT_TARGET ${component_lib} PARENT_SCOPE)

    __component_set_properties()
endfunction()


#
# Deprecated functions
#

# register_component
#
# Compatibility function for registering 3.xx style components.
macro(register_component)
    spaces2list(COMPONENT_SRCS)
    spaces2list(COMPONENT_SRCDIRS)
    spaces2list(COMPONENT_ADD_INCLUDEDIRS)
    spaces2list(COMPONENT_PRIV_INCLUDEDIRS)
    spaces2list(COMPONENT_REQUIRES)
    spaces2list(COMPONENT_PRIV_REQUIRES)
    spaces2list(COMPONENT_ADD_LDFRAGMENTS)
    spaces2list(COMPONENT_EMBED_FILES)
    spaces2list(COMPONENT_EMBED_TXTFILES)
    spaces2list(COMPONENT_SRCEXCLUDE)
    armino_component_register(SRCS "${COMPONENT_SRCS}"
                        SRC_DIRS "${COMPONENT_SRCDIRS}"
                        INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
                        PRIV_INCLUDE_DIRS "${COMPONENT_PRIV_INCLUDEDIRS}"
                        REQUIRES "${COMPONENT_REQUIRES}"
                        PRIV_REQUIRES "${COMPONENT_PRIV_REQUIRES}"
                        LDFRAGMENTS "${COMPONENT_ADD_LDFRAGMENTS}"
                        EMBED_FILES "${COMPONENT_EMBED_FILES}"
                        EMBED_TXTFILES "${COMPONENT_EMBED_TXTFILES}"
                        EXCLUDE_SRCS "${COMPONENT_SRCEXCLUDE}")
endmacro()

# require_armino_targets
#
# Compatibility function for requiring ARMINO build targets for 3.xx style components.
function(require_armino_targets)
    set(__REQUIRED_ARMINO_SOCS "${ARGN}")
    __component_check_target()
endfunction()

# register_config_only_component
#
# Compatibility function for registering 3.xx style config components.
macro(register_config_only_component)
    register_component()
endmacro()

# component_compile_options
#
# Wrapper around target_compile_options that passes the component name
function(component_compile_options)
    target_compile_options(${COMPONENT_LIB} PRIVATE ${ARGV})
endfunction()

# component_compile_definitions
#
# Wrapper around target_compile_definitions that passes the component name
function(component_compile_definitions)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${ARGV})
endfunction()

#
# armino_component_get_target
#
# armino component get target by component name
function(armino_component_get_target target name)
    # The component target has three underscores as a prefix.
    # The corresponding component library only has two.
    armino_build_get_property(prefix __PREFIX)
    # set(component_lib     __${prefix}_${name} PARENT_SCOPE)
    set(target __${prefix}_${name} PARENT_SCOPE)

endfunction()