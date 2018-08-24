#include <QCoreApplication>
#include "skedplayer-server.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    new SkedPlayerServer(&app);
    app.exec();
}
