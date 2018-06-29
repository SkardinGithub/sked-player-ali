#include "skedplayer.h"
#include <QtCore>
extern "C" {
#include <goplayer.h>
#include <alislsnd.h>
}

SkedPlayer * SkedPlayer::m_instance = NULL;

void _callback(eGOPLAYER_CALLBACK_TYPE type, void *data);

SkedPlayer::SkedPlayer(QObject *parent) : QObject(parent)
{
  qDebug() << "skedplayer constructor";
  if (SkedPlayer::m_instance)
    return;
  m_state = STATE_STOP;
  m_playback_rate = 1;
  m_fullscreen = false;
  m_displayrect = QRect(0, 0, 1280, 720);
  SkedPlayer::m_instance = this;
}

SkedPlayer::~SkedPlayer()
{
  qDebug() << "skedplayer destructor";
  goplayer_close();
}

void SkedPlayer::setSrc(const QString &src)
{
  qDebug() << "skedplayer set source" << src;
  goplayer_close();
  m_src = src;
  m_playback_rate = 1;
  m_start_time = 0;
  m_state = STATE_STOP;
  emit rateChange();
  emit stateChange();
}

void SkedPlayer::stop()
{
  qDebug() << "skedplayer stop";

  // measure for bug#8006
  QElapsedTimer timer;
  timer.start();
  goplayer_close();
  qDebug() << "\n\n\n\n\ngoplayer_close took" << timer.elapsed() << "milliseconds\n\n\n\n\n";

  m_playback_rate = 1;
  m_state = STATE_STOP;
  emit stateChange();
}

void SkedPlayer::load()
{
  qDebug() << "skedplayer load" << m_src << "from position" << m_start_time;
  if (m_src.isEmpty()) return;
  goplayer_close();

  // measure for bug#8006
  QElapsedTimer timer;
  timer.start();
  goplayer_open(_callback);
  qDebug() << "\n\n\n\n\ngoplayer_open took" << timer.elapsed() << "milliseconds\n\n\n\n\n";

  goplayer_set_source_uri(qPrintable(m_src), m_start_time * 1000, eSTREAM_PROTOCOL_UNKNOW);
  if (! m_fullscreen) goplayer_set_display_rect(m_displayrect.left(), m_displayrect.top(), m_displayrect.width(), m_displayrect.height());
  goplayer_set_subtitle_display(false);
  m_start_time = 0;
  m_duration = -1;
  goplayer_play(0);
  m_state = STATE_LOADED;
  emit stateChange();
}

void SkedPlayer::play()
{
  qDebug() << "skedplayer play";
  if (m_state == STATE_PLAY) return;
  if (m_src.isEmpty()) return;
  if (m_state == STATE_STOP || m_state == STATE_ENDED) load();
  goplayer_play(m_playback_rate);
  m_state = STATE_PLAY;
  emit stateChange();
}

void SkedPlayer::pause()
{
  qDebug() << "skedplayer pause";
  if (m_state == STATE_LOADED || m_state == STATE_PLAY) {
    goplayer_play(0);
    m_state = STATE_PAUSED;
    emit stateChange();
  }
}

double SkedPlayer::getCurrentTime()
{
  if (m_state == STATE_STOP) return -1;
  if (m_state == STATE_ENDED) return m_duration;
  return goplayer_get_current_time() / 1000.0;
}

bool SkedPlayer::seekable()
{
  return goplayer_is_seekable();
}

void SkedPlayer::setCurrentTime(double time)
{
  qDebug() << "skedplayer setCurrentTime" << time;
  switch (m_state) {
  case STATE_ENDED:
    m_start_time = time;
    play();
    break;
  case STATE_LOADED:
  case STATE_PAUSED:
  case STATE_PLAY:
    goplayer_seek(time * 1000);
    break;
  case STATE_STOP:
    m_start_time = time;
    break;
  default:
    break;
  }
}

void SkedPlayer::setVolume(double vol)
{
  qDebug() << "skedplayer setVolume" << vol;
  void *hsnd;
  if (0 == alislsnd_open(&hsnd)) {
    alislsnd_set_volume(hsnd, uint8_t(vol * 100), SND_IO_ALL);
    alislsnd_close(hsnd);
    emit volumeChange();
  } else {
    qWarning() << "skedplayer can not open snd";
  }
}

double SkedPlayer::getVolume()
{
  void *hsnd;
  if (0 == alislsnd_open(&hsnd)) {
    uint8_t vol;
    alislsnd_get_volume(hsnd, SND_IO_ALL, &vol);
    alislsnd_close(hsnd);
    return double(vol) / 100.0;
  }

  qWarning() << "skedplayer can not open snd";
  return -1.0;
}

void SkedPlayer::mute(bool mute)
{
  qDebug() << "skedplayer " << (mute ? "mute" : "unmute");
  void *hsnd;
  if (0 == alislsnd_open(&hsnd)) {
    alislsnd_set_mute(hsnd, mute, SND_IO_ALL);
    alislsnd_close(hsnd);
    emit volumeChange();
  } else {
    qWarning() << "skedplayer can not open snd";
  }
}

bool SkedPlayer::muted()
{
  void *hsnd;
  if (0 == alislsnd_open(&hsnd)) {
    bool mute;
    alislsnd_get_mute_state(hsnd, SND_IO_ALL, &mute);
    alislsnd_close(hsnd);
    return mute;
  }
  qWarning() << "skedplayer can not open snd";
  return false;
}

double SkedPlayer::duration()
{
  if (m_state == STATE_STOP) return -1;
  if (m_state == STATE_ENDED) return m_duration;
  if (m_duration != -1) return m_duration;
  int duration = goplayer_get_total_time();
  if (duration > 0) m_duration = duration / 1000;
  return m_duration;
}

void SkedPlayer::setPlayBackRate(double rate)
{
  qDebug() << "skedplayer set rate" << rate;
  // range [-128~-2, 0~128] in integer is valid
  int r = rate;
  if ((r < -128) || (r > -2 && r < 0) || (r > 128)) {
    qWarning() << "skedplayer invalid rate. rate should be integer in range [-128~-2, 0~128]";
    return;
  }
  m_playback_rate = r;
  if (m_state == STATE_PLAY) goplayer_play(m_playback_rate);
  emit rateChange();
}

void SkedPlayer::onEnded()
{
  qDebug() << "skedplayer ended";
  m_state = STATE_ENDED;
  emit stateChange();
}

void SkedPlayer::setDisplayRect(const QRect & rect)
{
  qDebug() << "skedplayer set display rect" << rect;
  m_displayrect = rect;
  if (! m_fullscreen) goplayer_set_display_rect(rect.left(), rect.top(), rect.width(), rect.height());
  emit displayRectChange();
}

void SkedPlayer::setFullScreen(bool full)
{
  qDebug() << "skedplayer" << (full ? "enter" : "leave") << "fullscreen";
  m_fullscreen = full;
  QRect rect = (m_fullscreen ? QRect(0, 0, 1280, 720) : m_displayrect);
  goplayer_set_display_rect(rect.left(), rect.top(), rect.width(), rect.height());
  emit displayRectChange();
}

void _callback(eGOPLAYER_CALLBACK_TYPE type, void *data)
{
  switch (type) {
  case eGOPLAYER_CBT_STATE_CHANGE: {
    eGOPLAYER_STATE state = (eGOPLAYER_STATE)(int)data;
    if(state == eGOPLAYER_STATE_PAUSE) {
      qDebug() << "[callback] state: pause";
    } else if (state == eGOPLAYER_STATE_PLAY) {
      qDebug() << "[callback] state: play";
    } else {
      qDebug() << "[callback] state: stop";
    }
  }
    break;

  case eGOPLAYER_CBT_BUFFERING: {
    int percent = (int)data;
    qDebug() << "[callback] buffering:" << percent;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "buffering", Qt::QueuedConnection, Q_ARG(int, percent));
  }
    break;

  case eGOPLAYER_CBT_WARN_UNSUPPORT_AUDIO:
    qWarning() << "[callback] UNSUPPORT AUDIO";
    break;

  case eGOPLAYER_CBT_WARN_UNSUPPORT_VIDEO:
    qWarning() << "[callback] UNSUPPORT VIDEO";
    break;

  case eGOPLAYER_CBT_WARN_DECODE_ERR_AUDIO:
    qWarning() << "[callback] AUDIO DECODE ERROR";
    break;

  case eGOPLAYER_CBT_WARN_DECODE_ERR_VIDEO:
    qWarning() << "[callback] VIDEO DECODE ERROR";
    break;

  case eGOPLAYER_CBT_WARN_TRICK_BOS:
    qWarning() << "[callback] RW to BOS";
    break;

  case eGOPLAYER_CBT_WARN_TRICK_EOS:
    qWarning() << "[callback] FF to EOS";
    break;

  case eGOPLAYER_CBT_ERR_SOUPHTTP:
    qWarning() << "[callback] Soup http error, code:" << (int)data;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_NETWORK));
    break;

  case eGOPLAYER_CBT_ERR_TYPE_NOT_FOUND:
  case eGOPLAYER_CBT_ERR_DEMUX:
  case eGOPLAYER_CBT_ERR_UNDEFINED:
    qWarning() << "[callback] error:" << (char *)data;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_SRC_NOT_SUPPORTED));
    break;

  case eGOPLAYER_CBT_FINISHED:
    qDebug() << "[callback] EOS";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "onEnded", Qt::QueuedConnection);
    break;

  case eGOPLAYER_CBT_FRAME_CAPTURE:
    break;
  case eGOPLAYER_CBT_MAX:
    break;
  default:
    break;
  }
}
