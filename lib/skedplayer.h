#ifndef SKEDPLAYER_H
#define SKEDPLAYER_H
#include <QObject>
#include <QtCore>

class SkedPlayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int state READ state NOTIFY stateChange)
    Q_PROPERTY(QString src READ getSrc WRITE setSrc)
    Q_PROPERTY(bool seekable READ seekable)
    Q_PROPERTY(double currentTime READ getCurrentTime WRITE setCurrentTime)
    Q_PROPERTY(double vol READ getVolume WRITE setVolume NOTIFY volumeChange)
    Q_PROPERTY(bool mute READ muted WRITE mute NOTIFY volumeChange)
    Q_PROPERTY(double rate READ getPlayBackRate WRITE setPlayBackRate NOTIFY rateChange)
    Q_PROPERTY(double duration READ duration)
    Q_PROPERTY(QRect displayrect READ getDisplayRect WRITE setDisplayRect NOTIFY displayRectChange)
    Q_PROPERTY(bool fullscreen READ getFullScreen WRITE setFullScreen NOTIFY displayRectChange)
    Q_PROPERTY(int bufferLevel READ getBufferLevel NOTIFY buffering)
    Q_PROPERTY(QVariantList videoTracks READ videoTracks)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks)
    Q_PROPERTY(int currentAudioTrack WRITE setCurrentAudioTrack)
    Q_PROPERTY(int currentSubtitleTrack WRITE setCurrentSubtitleTrack)

public:
    explicit SkedPlayer(QObject *parent = 0);
    ~SkedPlayer();
    static SkedPlayer *singleton(void) { return m_instance; }
    Q_INVOKABLE void load();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE bool stop();
    Q_INVOKABLE static double getLastStopTime(const QString &src);

    enum STATE {
      STATE_STOP,
      STATE_LOADED,
      STATE_PAUSED,
      STATE_PLAY,
      STATE_ENDED,
    };
    Q_ENUM(STATE)

    enum ERROR {
      ERROR_NETWORK = 1,
      ERROR_DECODE,
      ERROR_SRC_NOT_SUPPORTED,
      ERROR_SYSTEM,
    };
    Q_ENUM(ERROR)

signals:
    void stateChange(int oldState, int newState);
    void rateChange(double rate);
    void volumeChange(bool mute, double vol);
    void displayRectChange(bool fullscreen, const QRect & rect);
    void buffering(int percent);
    void error(int type);

private slots:
    void onEnded();
    void onStart();
    void onBuffering(int percent);

private:
    QString getSrc() const { return m_src; }
    void setSrc(const QString &src);
    double duration();
    double getCurrentTime();
    void setCurrentTime(double time);
    void setPlayBackRate(double rate);
    double getPlayBackRate() { return m_playback_rate; }
    void setVolume(double vol);
    double getVolume();
    void mute(bool mute);
    bool muted();
    int state() { return m_state; }
    QRect getDisplayRect() { return m_displayrect; }
    void setDisplayRect(const QRect & rect);
    bool getFullScreen() { return m_fullscreen; }
    void setFullScreen(bool full);
    bool seekable();
    int getBufferLevel() { return m_buffer_level; }
    QVariantList audioTracks();
    QVariantList videoTracks();
    QVariantList subtitleTracks();
    bool setCurrentAudioTrack(int track_index);
    bool setCurrentSubtitleTrack(int track_index);
    void displayFillBlack();
    void displayEnableVideo(bool on);
    void soundSetOutMode();
    void saveStopTime();
    void clearStopTime();

private:
    static SkedPlayer *m_instance;
    QString m_src;
    enum STATE m_state;
    double m_playback_rate;
    QRect m_displayrect;
    bool m_fullscreen;
    double m_start_time;
    double m_duration;
    bool m_inited;
    void *m_mp_handle;
    int m_buffer_level;
    QVariantList m_audio_tracks;
    QVariantList m_video_tracks;
    QVariantList m_subtitle_tracks;
};
#endif // SKEDPLAYER_H
