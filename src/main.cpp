
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("Hospital Management System");
    QApplication::setOrganizationName("Hospital Management");

    MainWindow window;
    window.show();

    return app.exec();
}
