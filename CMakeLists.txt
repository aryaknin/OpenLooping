cmake_minimum_required(VERSION 3.24)
project(OpenLooping VERSION 0.6.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets Test)

add_library(looping_core
    src/core/loopingimporter.cpp
    src/core/model.cpp
    src/core/persistence.cpp
    src/core/relationalgenerator.cpp
)
target_include_directories(looping_core PUBLIC src)
target_link_libraries(looping_core PUBLIC Qt6::Core)

add_executable(openlooping
    src/main.cpp
    src/ui/diagramview.cpp
    src/ui/mainwindow.cpp
    src/ui/objectdialog.cpp
)
target_link_libraries(openlooping PRIVATE looping_core Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    add_executable(looping_core_tests tests/core_tests.cpp)
    target_link_libraries(looping_core_tests PRIVATE looping_core Qt6::Test)
    target_compile_definitions(looping_core_tests PRIVATE
        LOOPING_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/work/analysis-raw")
    add_test(NAME looping_core_tests COMMAND looping_core_tests)

    add_executable(looping_ui_tests tests/ui_tests.cpp src/ui/objectdialog.cpp)
    target_link_libraries(looping_ui_tests PRIVATE looping_core Qt6::Widgets Qt6::Test)
    add_test(NAME looping_ui_tests COMMAND looping_ui_tests)
    set_tests_properties(looping_ui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

install(TARGETS openlooping RUNTIME DESTINATION bin)
install(FILES packaging/openlooping.desktop DESTINATION share/applications)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_NAME openlooping)
set(CPACK_PACKAGE_CONTACT "OpenLooping contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Editeur MCD libre pour Debian et Ubuntu")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6 (>= 6.4)")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)
