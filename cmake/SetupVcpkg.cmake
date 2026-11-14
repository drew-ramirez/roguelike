cmake_minimum_required(VERSION 3.21)

get_filename_component(project_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(deps_dir "${project_dir}/.deps")
set(vcpkg_dir "${deps_dir}/vcpkg")
set(origin "https://github.com/microsoft/vcpkg.git")
file(READ "${project_dir}/vcpkg.json" manifest)
string(JSON baseline ERROR_VARIABLE json_error GET "${manifest}" builtin-baseline)
if(json_error OR NOT baseline MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "vcpkg.json must contain a valid builtin-baseline commit.")
endif()
string(LENGTH "${baseline}" baseline_length)
if(NOT baseline_length EQUAL 40)
    message(FATAL_ERROR "The vcpkg builtin-baseline must be a full 40-character commit SHA.")
endif()
find_program(GIT_EXECUTABLE NAMES git REQUIRED)

if(IS_SYMLINK "${deps_dir}" OR IS_SYMLINK "${vcpkg_dir}")
    message(FATAL_ERROR "Refusing symlinked .deps or .deps/vcpkg. Use real directories.")
endif()

if(NOT EXISTS "${vcpkg_dir}")
    file(MAKE_DIRECTORY "${deps_dir}")
    execute_process(COMMAND "${GIT_EXECUTABLE}" clone --filter=blob:none --no-checkout
                    "${origin}" "${vcpkg_dir}" RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Could not clone vcpkg. Check Git and network access; inspect ${vcpkg_dir} before retrying.")
    endif()
    set(new_checkout TRUE)
else()
    if(NOT IS_DIRECTORY "${vcpkg_dir}/.git" OR
       NOT EXISTS "${vcpkg_dir}/scripts/buildsystems/vcpkg.cmake" OR
       NOT EXISTS "${vcpkg_dir}/bootstrap-vcpkg.bat")
        message(FATAL_ERROR "${vcpkg_dir} is not a standalone vcpkg Git checkout. Move it aside before retrying.")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" remote get-url origin
                    WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result
                    OUTPUT_VARIABLE actual_origin OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT result EQUAL 0 OR NOT actual_origin STREQUAL origin)
        message(FATAL_ERROR "${vcpkg_dir} has an unexpected Git origin. Inspect it before retrying.")
    endif()
    execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=all
                    WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result
                    OUTPUT_VARIABLE status OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT result EQUAL 0 OR NOT status STREQUAL "")
        message(FATAL_ERROR "${vcpkg_dir} has local changes or could not be inspected. Preserve or remove those changes before retrying.")
    endif()
endif()

execute_process(COMMAND "${GIT_EXECUTABLE}" cat-file -e "${baseline}^{commit}"
                WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result
                ERROR_QUIET)
if(NOT result EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" fetch origin
                    WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Could not fetch the vcpkg baseline. Check network access and retry.")
    endif()
endif()
execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
                WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE head_result
                OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND "${GIT_EXECUTABLE}" symbolic-ref -q HEAD
                WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE branch_result
                OUTPUT_QUIET ERROR_QUIET)
if(new_checkout OR NOT head_result EQUAL 0 OR NOT head STREQUAL baseline OR
   branch_result EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" checkout --detach "${baseline}"
                    WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Could not check out vcpkg baseline ${baseline}; inspect the checkout and retry.")
    endif()
endif()

if(CMAKE_HOST_WIN32)
    execute_process(COMMAND cmd.exe /d /c bootstrap-vcpkg.bat -disableMetrics
                    WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result)
else()
    execute_process(COMMAND sh ./bootstrap-vcpkg.sh -disableMetrics
                    WORKING_DIRECTORY "${vcpkg_dir}" RESULT_VARIABLE result)
endif()
if(NOT result EQUAL 0)
    message(FATAL_ERROR "vcpkg bootstrap failed. Check the output above and retry setup.")
endif()
message(STATUS "Project-local vcpkg is ready at ${vcpkg_dir}")
