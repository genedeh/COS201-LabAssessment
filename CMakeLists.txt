cmake_minimum_required(VERSION 3.21)
project(SmartClinicManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

add_executable(scapms
    main.cpp
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

configure_file(data/clinic_seed.json ${CMAKE_CURRENT_BINARY_DIR}/clinic_seed.json COPYONLY)
