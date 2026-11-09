#include "ui/mainwindow.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setApplicationName("OpenLooping");
    QApplication::setOrganizationName("OpenLooping");
    MainWindow window; window.show();
    if (argc > 1) window.loadModel(QString::fromLocal8Bit(argv[1]));
    return app.exec();
}
