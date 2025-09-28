cmake_minimum_required(VERSION 3.16)
project(NeuVCPlugin VERSION 0.1.0)

# C++ settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable verbose makefile
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set the location of libtorch
set(Torch_DIR "C:/libtorch/share/cmake/Torch")
find_package(Torch REQUIRED)

# Export compile_commands.json (utile in VS Code to Compile commands for IDEs)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Static linking on Windows
if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Add JUCE
add_subdirectory(ThirdParty/JUCE)

set(BaseTargetName NeuVC)

# Set COPY_PLUGIN_AFTER_BUILD based on OS
if (APPLE)
    set(COPY_PLUGIN_AFTER_BUILD TRUE)
else()
    set(COPY_PLUGIN_AFTER_BUILD FALSE)
endif()

juce_set_vst2_sdk_path("C:/SDKs/vstsdk2.4")
juce_set_aax_sdk_path("C:/SDKs/aax-sdk")

include_directories(C:/SDKs/asiosdk/common)
include_directories("C:/Program Files/JACK2/include")
include_directories(${CMAKE_CURRENT_LIST_DIR}/ThirdParty/dlfcn/src)

juce_add_plugin("${BaseTargetName}"
    ICON_BIG "${CMAKE_CURRENT_LIST_DIR}/NeuVC/Assets/logo.png"
    ICON_SMALL "${CMAKE_CURRENT_LIST_DIR}/NeuVC/Assets/logo.png"
    
    COMPANY_NAME guglielmofratticioli
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_AFTER_BUILD}
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Need access to Microphone"
    HARDENED_RUNTIME_ENABLED TRUE
    PLUGIN_MANUFACTURER_CODE DrAu
    PLUGIN_CODE NRNT
    FORMATS VST VST3 Standalone AAX LV2
    VST3_CATEGORIES "Fx"
    VST2_CATEGORY kPlugCategEffect
    AAX_ePlugInCategory_Effect
    BUNDLE_ID "com.guglielmofratticioli.NeuVC"
    LV2URI https://github.com/guglielmofratticioli/NeuVC
    AAX_IDENTIFIER com.guglielmofratticioli.NeuVC
    PRODUCT_NAME "NeuVC"
)

juce_generate_juce_header(${BaseTargetName})

# Source files
file(GLOB_RECURSE SOURCES_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/NeuVC/*.cpp ${CMAKE_CURRENT_LIST_DIR}/Lib/*.cpp)
file(GLOB_RECURSE HEADERS_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/NeuVC/*.h ${CMAKE_CURRENT_LIST_DIR}/Lib/*.h)

target_sources(${BaseTargetName} PRIVATE ${SOURCES_PLUGIN} ${HEADERS_PLUGIN})

# Include directories (Include all subdirectories in /Lib and /NeuVC)
file(GLOB_RECURSE lib_dirs LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/Lib/*)
foreach (dir ${lib_dirs})
    if (IS_DIRECTORY ${dir})
        target_include_directories(${BaseTargetName} PRIVATE ${dir})
    endif()
endforeach()

file(GLOB_RECURSE source_dirs LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/NeuVC/*)
foreach (dir ${source_dirs})
    if (IS_DIRECTORY ${dir})
        target_include_directories(${BaseTargetName} PRIVATE ${dir})
    endif()
endforeach()

# Aggiungi risorse .rc su Windows
if (MSVC)
    target_sources(${BaseTargetName} PRIVATE NeuVC/resources.rc)
endif()


# Binary data
file(GLOB RESOURCES_FILES
    ${CMAKE_CURRENT_LIST_DIR}/Lib/ModelData/*.json
    ${CMAKE_CURRENT_LIST_DIR}/Lib/ModelData/*.ort
    ${CMAKE_CURRENT_LIST_DIR}/NeuVC/Assets/*.ttf
    ${CMAKE_CURRENT_LIST_DIR}/NeuVC/Assets/*.png
    ${CMAKE_CURRENT_LIST_DIR}/NeuVC/Assets/*.svg
)
juce_add_binary_data(bin_data SOURCES ${RESOURCES_FILES})

target_include_directories(${BaseTargetName} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ThirdParty/minimp3)

target_compile_definitions(${BaseTargetName}
    PRIVATE
        _WIN32_WINNT=0x0A00
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_ASIO=1
        JUCE_JACK=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link libraries
target_link_libraries(${BaseTargetName}
  PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    bin_data
    ${TORCH_LIBRARIES}
  PUBLIC
    juce_recommended_config_flags
    juce_recommended_lto_flags
)

# Add executable for test_rvc (spostato tutto insieme)
add_executable(test_rvc
    NeuVC/Source/RVC/main.cpp
    NeuVC/Source/RVC/rvc.cpp
)

# Include RVC header directories
target_include_directories(test_rvc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/NeuVC/Source/RVC )

# Include Torch directories for test_rvc
target_include_directories(test_rvc PRIVATE ${TORCH_INCLUDE_DIRS})

# Link Torch libraries for test_rvc
target_link_libraries(test_rvc PRIVATE ${TORCH_LIBRARIES})

# Set C++ standard for test_rvc
set_property(TARGET test_rvc PROPERTY CXX_STANDARD 17)

# Copy Torch DLLs on Windows
if (MSVC)
    file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET ${BaseTargetName}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TORCH_DLLS}
        $<TARGET_FILE_DIR:${BaseTargetName}>
    )
    add_custom_command(TARGET test_rvc
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TORCH_DLLS}
        $<TARGET_FILE_DIR:test_rvc>
    )
endif()
