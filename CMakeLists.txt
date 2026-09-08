cmake_minimum_required(VERSION 3.24)

project(NNMTMPlatform VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Torch REQUIRED)

qt_standard_project_setup()

qt_add_executable(NNMTMPlatform
    src/app/main.cpp
    src/ui/AppUI.h
    src/ui/ModelCard.cpp
    src/ui/ModelManagementDialogs.cpp
    src/ui/TrainingWindow.cpp
    src/ui/AddModelDialog.cpp
    src/ui/UseModelDialog.cpp
    src/ui/AnalysisDialogs.cpp
    src/ui/MainWindow.cpp
    src/core/ModelCore.h
    src/models/NeuralNetwork.h
    src/models/ANN.h
    src/models/ANNModels.h
    src/models/CNN.h
    src/models/CNNModels.h
)

target_include_directories(NNMTMPlatform PRIVATE
    src/ui
    src/core
    src/models
)

target_link_libraries(NNMTMPlatform PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    ${TORCH_LIBRARIES}
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
)

if(TORCH_CXX_FLAGS)
    separate_arguments(TORCH_CXX_FLAGS_LIST NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    target_compile_options(NNMTMPlatform PRIVATE ${TORCH_CXX_FLAGS_LIST})
endif()

if(MSVC)
    target_compile_definitions(NNMTMPlatform PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
    target_compile_options(NNMTMPlatform PRIVATE /utf-8 /W3 /permissive-)
endif()

set_target_properties(NNMTMPlatform PROPERTIES
    WIN32_EXECUTABLE ON
)

if(WIN32)
    file(GLOB TORCH_RUNTIME_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    if(TORCH_RUNTIME_DLLS)
        add_custom_command(TARGET NNMTMPlatform POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${TORCH_RUNTIME_DLLS}
                $<TARGET_FILE_DIR:NNMTMPlatform>
            COMMAND_EXPAND_LISTS
            COMMENT "Deploying LibTorch runtime libraries"
        )
    endif()

    foreach(OPENCV_TARGET IN ITEMS opencv_core opencv_imgproc opencv_imgcodecs)
        if(TARGET ${OPENCV_TARGET})
            add_custom_command(TARGET NNMTMPlatform POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${OPENCV_TARGET}>
                    $<TARGET_FILE_DIR:NNMTMPlatform>
                COMMENT "Deploying ${OPENCV_TARGET} runtime library"
            )
        endif()
    endforeach()

    if(TARGET Qt6::windeployqt)
        add_custom_command(TARGET NNMTMPlatform POST_BUILD
            COMMAND $<TARGET_FILE:Qt6::windeployqt>
                --no-translations
                --no-system-d3d-compiler
                $<TARGET_FILE:NNMTMPlatform>
            COMMENT "Deploying Qt runtime libraries"
        )
    endif()
endif()

include(CTest)

install(TARGETS NNMTMPlatform
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)
