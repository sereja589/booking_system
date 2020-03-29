#include "start_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TStartWindow w;
    w.show();
    return a.exec();
}
