#ifndef SKEDPLAYER_SERVER_H
#define SKEDPLAYER_SERVER_H
#include <QCoreApplication>
#include <QtDBus>
#include "../lib/skedplayer.h"

class SkedPlayerServer : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.sked.service.player")

    Q_PROPERTY(int state READ state)
    Q_PROPERTY(QString src READ getSrc WRITE setSrc)
    Q_PROPERTY(bool seekable READ seekable)
    Q_PROPERTY(double currentTime READ getCurrentTime WRITE setCurrentTime)
    Q_PROPERTY(double vol READ getVolume WRITE setVolume)
    Q_PROPERTY(bool mute READ muted WRITE mute)
    Q_PROPERTY(double rate READ getPlayBackRate WRITE setPlayBackRate)
    Q_PROPERTY(double duration READ duration)
    Q_PROPERTY(QRect displayrect READ getDisplayRect WRITE setDisplayRect)
    Q_PROPERTY(bool fullscreen READ getFullScreen WRITE setFullScreen)
    Q_PROPERTY(int bufferLevel READ getBufferLevel)

public:
    SkedPlayerServer(QCoreApplication *application) : QDBusAbstractAdaptor(application)
    {
        m_player = new SkedPlayer(this);
        connect(m_player, SIGNAL(stateChange(int,int)), SIGNAL(stateChange(int,int)));
        connect(m_player, SIGNAL(rateChange(double)), SIGNAL(rateChange(double)));
        connect(m_player, SIGNAL(volumeChange(bool,double)), SIGNAL(volumeChange(bool,double)));
        connect(m_player, SIGNAL(displayRectChange(bool,QRect)), SIGNAL(displayRectChange(bool,QRect)));
        connect(m_player, SIGNAL(buffering(int)), SIGNAL(buffering(int)));
        connect(m_player, SIGNAL(error(int)), SIGNAL(error(int)));
        QDBusConnection sys = QDBusConnection::systemBus();
        if (!sys.isConnected()) {
          qCritical("skedplayer-server - Failed to connect to system bus: %s", qPrintable(sys.lastError().message()));
          return;
        }
        if (!sys.registerService("com.sked.service.player")) {
          qCritical("skedplayer-server - Failed to register service 'com.sked.service.player': %s", qPrintable(sys.lastError().message()));
          return;
        }
        if (!sys.registerObject("/com/sked/service/player", application)) {
          qCritical("skedplayer-server - Failed to register object '/com/sked/service/player': %s", qPrintable(sys.lastError().message()));
          return;
        }
    }
    ~SkedPlayerServer() { delete m_player; }

public slots:
    Q_NOREPLY void load() { m_player->load(); }
    Q_NOREPLY void play() { m_player->play(); }
    Q_NOREPLY void pause() { m_player->pause(); }
    Q_NOREPLY bool stop() { return m_player->stop(); }

signals:
    void stateChange(int oldState, int newState);
    void rateChange(double rate);
    void volumeChange(bool mute, double vol);
    void displayRectChange(bool fullscreen, const QRect & rect);
    void buffering(int percent);
    void error(int type);

private:
    QString getSrc() const { return m_player->property("src").toString(); }
    void setSrc(const QString &src) { m_player->setProperty("src", src); }
    double duration() { return m_player->property("duration").toDouble(); }
    double getCurrentTime() { return m_player->property("currentTime").toDouble(); }
    void setCurrentTime(double time) { m_player->setProperty("currentTime", time); }
    void setPlayBackRate(double rate) { m_player->setProperty("rate", rate); }
    double getPlayBackRate() { return m_player->property("rate").toDouble(); }
    void setVolume(double vol) { m_player->setProperty("vol", vol); }
    double getVolume() { return m_player->property("vol").toDouble(); }
    void mute(bool mute) { m_player->setProperty("mute", mute); }
    bool muted() { return m_player->property("mute").toBool(); }
    int state() { return m_player->property("state").toInt(); }
    QRect getDisplayRect() { return m_player->property("displayrect").toRect(); }
    void setDisplayRect(const QRect & rect) { m_player->setProperty("displayrect", rect); }
    bool getFullScreen() { return m_player->property("fullscreen").toBool(); }
    void setFullScreen(bool full) { m_player->setProperty("fullscreen", full); }
    bool seekable() { return m_player->property("seekable").toBool(); }
    int getBufferLevel() { return m_player->property("bufferLevel").toInt(); }

private:
    SkedPlayer *m_player;
};
#endif // SKEDPLAYER_SERVER_H
