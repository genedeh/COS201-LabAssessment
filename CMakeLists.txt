cmake_minimum_required(VERSION 3.21)
project(SmartClinicManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
find_package(Qt6 6.4 QUIET COMPONENTS Svg)

add_executable(scapms
    main.cpp
    resources/resources.qrc
    gui/MainWindow.cpp
    gui/MainWindow.h
    model/Appointment.h
    model/Appointable.h
    model/AdminAction.h
    model/AdminActionHistory.h
    model/ClinicDatabase.h
    model/EmergencyPatient.h
    model/FollowUpPatient.h
    model/GeneralPatient.h
    model/PatientAccessCache.h
    model/PatientRecord.h
    model/WaitingQueue.h
    controller/AdminController.h
    controller/AppointmentController.h
    controller/ClinicManager.h
    controller/PatientController.h
    controller/ReportController.h
    controller/SearchEngine.h
    utils/ClinicException.h
    utils/DateUtils.h
    utils/FileHandler.h
    utils/IDGenerator.h
)

target_include_directories(scapms PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scapms PRIVATE Qt6::Widgets)
if(TARGET Qt6::Svg)
    target_link_libraries(scapms PRIVATE Qt6::Svg)
    target_compile_definitions(scapms PRIVATE SCAPMS_HAS_QT_SVG=1)
endif()

configure_file(data/clinic_seed.json ${CMAKE_CURRENT_BINARY_DIR}/clinic_seed.json COPYONLY)
