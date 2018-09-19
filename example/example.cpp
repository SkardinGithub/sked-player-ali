#include <QApplication>
#include <QtQuick>
#include <QtPlugin>
#include "../lib/skedplayer.h"
#include <QtCore/QDebug>

#ifdef QT_STATIC
Q_IMPORT_PLUGIN(QtQuick2Plugin)
Q_IMPORT_PLUGIN(QmlFolderListModelPlugin)
#endif

static QObject *skedplayer_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  return SkedPlayer::singleton();
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  new SkedPlayer();
  qmlRegisterSingletonType<SkedPlayer>("SKED.MediaPlayer", 1, 0, "Player", skedplayer_singletontype_provider);

  QQuickView view;
  view.setSource(QUrl("qrc:/qml/main.qml"));
  view.setColor(QColor(Qt::transparent));
  view.show();
  view.requestActivate();

  return app.exec();
}
