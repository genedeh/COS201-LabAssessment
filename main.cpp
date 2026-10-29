#include <QApplication>

#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName("Smart Clinic Appointment & Patient Management System");
    MainWindow window;
    window.showMaximized();
    return application.exec();
}
