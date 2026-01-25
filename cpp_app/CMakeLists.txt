cmake_minimum_required(VERSION 3.16)
project(limpia_imagenes_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Widgets Core QUIET)
if(Qt6_FOUND)
  set(QT_LIBS Qt6::Widgets Qt6::Core)
else()
  find_package(Qt5 COMPONENTS Widgets Core REQUIRED)
  set(QT_LIBS Qt5::Widgets Qt5::Core)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(denoise src/denoise.cpp)
target_include_directories(denoise PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(denoise PRIVATE ${OpenCV_LIBS})

add_executable(limpia_imagenes_cpp src/main.cpp)
target_link_libraries(limpia_imagenes_cpp PRIVATE denoise ${OpenCV_LIBS})

set(APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../assets/logo.icns)

add_executable(limpia_imagenes_gui src/gui_main.cpp ${APP_ICON})
target_link_libraries(limpia_imagenes_gui PRIVATE denoise ${OpenCV_LIBS} ${QT_LIBS})
if(APPLE)
  set_target_properties(limpia_imagenes_gui PROPERTIES
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "LimpiaImagenes"
    MACOSX_BUNDLE_NAME "LimpiaImagenes"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.sev.costadelsol.limpiaimagenes"
    MACOSX_BUNDLE_ICON_FILE "logo.icns"
    RESOURCE "${APP_ICON}"
  )
endif()
