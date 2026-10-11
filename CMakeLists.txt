cmake_minimum_required(VERSION 3.24)

project(NNMTMPlatform VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(Torch REQUIRED)

qt_standard_project_setup()

qt_add_executable(NNMTMPlatform
    src/app/main.cpp
    src/ui/AppUI.h
    src/ui/PlotWidget.h
    src/ui/ModelCard.cpp
    src/ui/ModelManagementDialogs.cpp
    src/ui/TrainingWindow.cpp
    src/ui/AddModelDialog.cpp
    src/ui/UseModelDialog.cpp
    src/ui/AnalysisDialogs.cpp
    src/ui/MainWindow.cpp
    src/core/ModelCore.h
    src/core/TrainingUtils.h
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

if(BUILD_TESTING)
    add_executable(NNMTMPlotRenderCheck tests/PlotRenderCheck.cpp src/ui/PlotWidget.h)
    target_include_directories(NNMTMPlotRenderCheck PRIVATE src/ui)
    target_link_libraries(NNMTMPlotRenderCheck PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(NNMTMPlotRenderCheck PRIVATE /utf-8 /W3 /permissive-)
    endif()
    add_test(NAME plot_render COMMAND NNMTMPlotRenderCheck "${CMAKE_BINARY_DIR}/plot-checks")
    if(WIN32)
        set_tests_properties(plot_render PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
    else()
        set_tests_properties(plot_render PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    add_executable(NNMTMCoreTests
        tests/CoreLogicTests.cpp
        src/core/TrainingUtils.h
    )
    target_include_directories(NNMTMCoreTests PRIVATE src/core)
    if(MSVC)
        target_compile_options(NNMTMCoreTests PRIVATE /utf-8 /W4 /permissive-)
    endif()
    add_test(NAME core_logic COMMAND NNMTMCoreTests)

    add_executable(NNMTMCudaRuntimeCheck tests/CudaRuntimeCheck.cpp)
    target_link_libraries(NNMTMCudaRuntimeCheck PRIVATE ${TORCH_LIBRARIES})
    if(TORCH_CXX_FLAGS)
        target_compile_options(NNMTMCudaRuntimeCheck PRIVATE ${TORCH_CXX_FLAGS_LIST})
    endif()
    if(MSVC)
        target_compile_options(NNMTMCudaRuntimeCheck PRIVATE /utf-8 /W3 /permissive-)
    endif()
    add_test(NAME cuda_runtime COMMAND NNMTMCudaRuntimeCheck)
    set_tests_properties(cuda_runtime PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(NNMTMAnnPipelineCheck
        tests/AnnPipelineCheck.cpp
        src/models/NeuralNetwork.h
        src/models/ANN.h
        src/models/ANNModels.h
        src/core/TrainingUtils.h
    )
    target_include_directories(NNMTMAnnPipelineCheck PRIVATE src/models src/core)
    target_link_libraries(NNMTMAnnPipelineCheck PRIVATE ${TORCH_LIBRARIES})
    if(TORCH_CXX_FLAGS)
        target_compile_options(NNMTMAnnPipelineCheck PRIVATE ${TORCH_CXX_FLAGS_LIST})
    endif()
    if(MSVC)
        target_compile_options(NNMTMAnnPipelineCheck PRIVATE /utf-8 /W3 /permissive-)
    endif()
    add_test(NAME ann_pipeline COMMAND NNMTMAnnPipelineCheck)

    add_executable(NNMTMCnnPipelineCheck
        tests/CnnPipelineCheck.cpp
        src/models/NeuralNetwork.h
        src/models/CNN.h
        src/models/CNNModels.h
        src/core/TrainingUtils.h
    )
    target_include_directories(NNMTMCnnPipelineCheck PRIVATE src/models src/core)
    target_link_libraries(NNMTMCnnPipelineCheck PRIVATE
        ${TORCH_LIBRARIES}
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
    )
    if(TORCH_CXX_FLAGS)
        target_compile_options(NNMTMCnnPipelineCheck PRIVATE ${TORCH_CXX_FLAGS_LIST})
    endif()
    if(MSVC)
        target_compile_options(NNMTMCnnPipelineCheck PRIVATE /utf-8 /W3 /permissive-)
    endif()
    add_test(NAME cnn_pipeline COMMAND NNMTMCnnPipelineCheck)

    if(WIN32)
        file(GLOB CUDA_TEST_RUNTIME_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
        if(CUDA_TEST_RUNTIME_DLLS)
            foreach(CUDA_TEST_TARGET IN ITEMS
                    NNMTMCudaRuntimeCheck
                    NNMTMAnnPipelineCheck
                    NNMTMCnnPipelineCheck)
                add_custom_command(TARGET ${CUDA_TEST_TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CUDA_TEST_RUNTIME_DLLS}
                        $<TARGET_FILE_DIR:${CUDA_TEST_TARGET}>
                    COMMAND_EXPAND_LISTS
                    COMMENT "Deploying LibTorch runtime libraries for ${CUDA_TEST_TARGET}"
                )
            endforeach()
        endif()
    endif()
endif()

install(TARGETS NNMTMPlatform
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)
