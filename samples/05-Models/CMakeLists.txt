cmake_minimum_required( VERSION 3.18.3 ) # Latest version of CMake when this file was created.

set( TARGET_NAME 05-Models )

set( HEADER_FILES
    inc/Camera.h
    inc/CameraController.h
    inc/EffectPSO.h
    inc/Light.h
    inc/SceneVisitor.h
    inc/Tutorial5.h
    inc/AppSettings.h
    inc/pathtrace.h
    inc/SceneStruct.h
)

set( SRC_FILES
    src/Camera.cpp
    src/CameraController.cpp
    src/EffectPSO.cpp
    src/Light.cpp
    src/main.cpp
    src/SceneVisitor.cpp
    src/Tutorial5.cpp
    src/PathtracePipeline.cpp
    src/AppSettings.cpp
)

set( VERTEX_SHADERS 
    shaders/Basic_VS.hlsl
)

set( PIXEL_SHADERS
    shaders/Decal_PS.hlsl
    shaders/Lighting_PS.hlsl
    shaders/Unlit_PS.hlsl
)

set( SHADER_INCLUDES
    shaders/Base_PS.hlsli
    shaders/BRDF.hlsl
    shaders/Clusters.hlsl
    shaders/RayTracing.hlsl
    shaders/Quaternion.hlsl
    shaders/SampleFunction.hlsl
    shaders/Constants.hlsl
    shaders/AppSettings.hlsl
    shaders/DescriptorTables.hlsl
    
)

set( SHADERS
    ${VERTEX_SHADERS}
    ${PIXEL_SHADERS}
    ${SHADER_INCLUDES}
)

source_group( "Resources\\Shaders" FILES ${SHADERS} )

set_source_files_properties( ${VERTEX_SHADERS} ${PIXEL_SHADERS}
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_SOURCE_DIR}/Data/Shaders/05-Models/%(Filename).cso"
        VS_SHADER_MODEL 6.0
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
        VS_SHADER_FLAGS $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-Qembed_debug>
)

set_source_files_properties( ${VERTEX_SHADERS}
    PROPERTIES 
        VS_SHADER_TYPE Vertex
)

set_source_files_properties( ${PIXEL_SHADERS}
    PROPERTIES 
        VS_SHADER_TYPE Pixel
)

add_executable( ${TARGET_NAME} WIN32
    ${HEADER_FILES} 
    ${SRC_FILES}
    ${SHADERS}
)

target_include_directories( ${TARGET_NAME}
    PRIVATE inc
)

target_link_libraries( ${TARGET_NAME}
    GameFramework
    SampleFramework12
    DX12Lib
    assimp
    d3dcompiler.lib
)

# Set Local Debugger Settings (Command Arguments and Environment Variables)
set( COMMAND_ARGUMENTS "-wd \"${CMAKE_SOURCE_DIR}\"" )
configure_file( ${TARGET_NAME}.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.vcxproj.user @ONLY )
