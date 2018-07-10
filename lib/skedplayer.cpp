#include "skedplayer.h"
#include <QtCore>
#include <stdlib.h>
extern "C" {
#include <aui_mp.h>
#include <alislsnd.h>
#include <aui_dis.h>
}

SkedPlayer * SkedPlayer::m_instance = NULL;

static void _callback(aui_mp_message type, void *data, void *userData);
static enum aui_mp_speed rateToAuiMpSpeed(double rate);

SkedPlayer::SkedPlayer(QObject *parent) : QObject(parent)
{
  qDebug() << "skedplayer constructor";
  if (SkedPlayer::m_instance)
    return;
  m_mp_handle = NULL;
  m_state = STATE_STOP;
  m_playback_rate = 1;
  m_fullscreen = true;
  m_displayrect = QRect(0, 0, 1280, 720);
  setenv("GST_REGISTRY", "/tmp/gst_registry.bin", 0);
  if (!getenv("OMX_BELLAGIO_REGISTRY")) {
    setenv("OMX_BELLAGIO_REGISTRY", "/tmp/omx_bellagio_registry", 0);
    QProcess::execute("omxregister-bellagio", QStringList() << "/usr/lib/bellagio/");
  }
  SkedPlayer::m_instance = this;
}

SkedPlayer::~SkedPlayer()
{
  qDebug() << "skedplayer destructor";
  if (m_mp_handle) {
    aui_mp_close(NULL, &m_mp_handle);
    m_mp_handle = NULL;
  }
}

void SkedPlayer::setSrc(const QString &src)
{
  qDebug() << "skedplayer set source" << src;
  m_src = src;
  m_start_time = 0;
  m_playback_rate = 1;
  m_fullscreen = true;
  if (m_state != STATE_STOP) {
    if (m_mp_handle) {
      aui_mp_close(NULL, &m_mp_handle);
      m_mp_handle = NULL;
    }
    displayEnableVideo(false);
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }
}

void SkedPlayer::stop()
{
  qDebug() << "skedplayer stop";
  m_playback_rate = 1;
  m_fullscreen = true;
  if (m_state != STATE_STOP) {
    QElapsedTimer timer; // measure for bug#8006
    timer.start();
    if (m_mp_handle) {
      aui_mp_close(NULL, &m_mp_handle);
      m_mp_handle = NULL;
    }
    qDebug() << "\n\n\n\n\naui_mp_close took" << timer.elapsed() << "milliseconds\n\n\n\n\n";
    displayEnableVideo(false);
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }
}

void SkedPlayer::load()
{
  qDebug() << "skedplayer load" << m_src << "from position" << m_start_time;
  if (m_state == STATE_LOADED) return;
  if (m_src.isEmpty()) return;
  m_inited = false;
  m_duration = -1;
  QElapsedTimer timer; // measure for bug#8006
  if (m_state != STATE_STOP) {
    timer.start();
    if (m_mp_handle) {
      aui_mp_close(NULL, &m_mp_handle);
      m_mp_handle = NULL;
    }
    qDebug() << "\n\n\n\n\naui_mp_close took" << timer.elapsed() << "milliseconds\n\n\n\n\n";
    displayEnableVideo(false);
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }
  displayFillBlack();
  timer.restart();
  aui_attr_mp mp_attr;
  memset(&mp_attr, 0, sizeof(mp_attr));
  strncpy((char *)mp_attr.uc_file_name, qPrintable(m_src), 1024);
  mp_attr.stream_protocol = AUI_MP_STREAM_PROTOCOL_UNKNOW;
  mp_attr.start_time = m_start_time * 1000;
  mp_attr.aui_mp_stream_cb = _callback;
  mp_attr.user_data = NULL;
  aui_mp_open(&mp_attr, &m_mp_handle);
  aui_mp_start(m_mp_handle);
  qDebug() << "\n\n\n\n\aui_mp_open + aui_mp_start took" << timer.elapsed() << "milliseconds\n\n\n\n\n";
  m_start_time = 0;
  emit rateChange(m_playback_rate);
  emit displayRectChange(m_fullscreen, m_displayrect);
  enum STATE oldState = m_state;
  m_state = STATE_LOADED;
  emit stateChange(oldState, m_state);
}

void SkedPlayer::play()
{
  qDebug() << "skedplayer play";
  if (m_state == STATE_PLAY) return;
  if (m_src.isEmpty()) return;
  if (m_state == STATE_STOP || m_state == STATE_ENDED) load();
  aui_mp_speed_set(m_mp_handle, rateToAuiMpSpeed(m_playback_rate));
  enum STATE oldState = m_state;
  m_state = STATE_PLAY;
  emit stateChange(oldState, m_state);
}

void SkedPlayer::pause()
{
  qDebug() << "skedplayer pause";
  if (m_state == STATE_LOADED || m_state == STATE_PLAY) {
    aui_mp_pause(m_mp_handle);
    enum STATE oldState = m_state;
    m_state = STATE_PAUSED;
    emit stateChange(oldState, m_state);
  }
}

double SkedPlayer::getCurrentTime()
{
  if (m_state == STATE_STOP) return -1;
  if (m_state == STATE_ENDED) return m_duration;
  unsigned int currentTime;
  if (0 != aui_mp_get_cur_time(m_mp_handle, &currentTime)) return -1;
  return currentTime / 1000.0;
}

bool SkedPlayer::seekable()
{
  if (m_state == STATE_STOP) return false;
  int isSeekable;
  if (0 != aui_mp_is_seekable(m_mp_handle, &isSeekable)) return false;
  return isSeekable;
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
    aui_mp_seek(m_mp_handle, time * 1000);
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
  // TODO: use aui API
  qDebug() << "skedplayer setVolume" << vol;
  void *hsnd;
  if (0 == alislsnd_open(&hsnd)) {
    bool mute;
    alislsnd_set_volume(hsnd, uint8_t(vol * 100), SND_IO_ALL);
    alislsnd_get_mute_state(hsnd, SND_IO_ALL, &mute);
    alislsnd_close(hsnd);
    emit volumeChange(mute, vol);
  } else {
    qWarning() << "skedplayer can not open snd";
  }
}

double SkedPlayer::getVolume()
{
  // TODO: use aui API
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
  // TODO: use aui API
  qDebug() << "skedplayer " << (mute ? "mute" : "unmute");
  void *hsnd;
  if (0 == alislsnd_open(&hsnd)) {
    uint8_t vol;
    alislsnd_set_mute(hsnd, mute, SND_IO_ALL);
    alislsnd_get_volume(hsnd, SND_IO_ALL, &vol);
    alislsnd_close(hsnd);
    emit volumeChange(mute, vol/100.0);
  } else {
    qWarning() << "skedplayer can not open snd";
  }
}

bool SkedPlayer::muted()
{
  // TODO: use aui API
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
  unsigned int total_time;
  if (0 != aui_mp_get_total_time(m_mp_handle, &total_time)) return m_duration;
  m_duration = total_time / 1000.0;
  return m_duration;
}

void SkedPlayer::setPlayBackRate(double rate)
{
  qDebug() << "skedplayer set rate" << rate;
  // range [-24~-2, 0~24] in integer is valid
  int r = rate;
  if ((r < -24) || (r > -2 && r < 0) || (r > 24)) {
    qWarning() << "skedplayer invalid rate. rate should be integer in range [-24~-2, 0~24]";
    return;
  }
  m_playback_rate = r;
  if (m_state == STATE_PLAY) aui_mp_speed_set(m_mp_handle, rateToAuiMpSpeed(m_playback_rate));
  emit rateChange(m_playback_rate);
}

void SkedPlayer::setDisplayRect(const QRect & rect)
{
  qDebug() << "skedplayer set display rect" << rect;
  m_displayrect = rect;
  // TODO
#if 0
  if (! m_fullscreen && m_state != STATE_STOP)
    goplayer_set_display_rect(rect.left(), rect.top(), rect.width(), rect.height());
#endif
  emit displayRectChange(m_fullscreen, m_displayrect);
}

void SkedPlayer::setFullScreen(bool full)
{
  qDebug() << "skedplayer" << "fullscreen" << full;
  m_fullscreen = full;
  QRect rect = (m_fullscreen ? QRect(0, 0, 1280, 720) : m_displayrect);
  // TODO
#if 0
  if (m_state != STATE_STOP)
    goplayer_set_display_rect(rect.left(), rect.top(), rect.width(), rect.height());
#else
  Q_UNUSED(rect);
#endif
  emit displayRectChange(m_fullscreen, m_displayrect);
}

void SkedPlayer::displayFillBlack()
{
  qDebug() << "skedplayer display fill black";
  void *dis_hdl_hd = 0;
  aui_attr_dis attr_dis_hd;
  memset(&attr_dis_hd, 0, sizeof(attr_dis_hd));
  attr_dis_hd.uc_dev_idx = AUI_DIS_HD;

  if(0 != aui_find_dev_by_idx(AUI_MODULE_DIS, 0, &dis_hdl_hd)) {
    if(0 != aui_dis_open(&attr_dis_hd, &dis_hdl_hd)) {
      qWarning() << "skedplayer aui_dis_open fail";
      return;
    }
  }

  if(0 != aui_dis_fill_black_screen(dis_hdl_hd)) {
    qWarning() << "skedplayer aui_dis_fill_black_screen fail";
  }

  if(0 != aui_dis_close(&attr_dis_hd, &dis_hdl_hd)) {
    qWarning() << "skedplayer aui_dis_close fail";
  }
}

void SkedPlayer::displayEnableVideo(bool on)
{
  qDebug() << "skedplayer" << (on ? "enable" : "disable") << "display";
  void *dis_hdl_hd = 0;
  aui_attr_dis attr_dis_hd;
  memset(&attr_dis_hd, 0, sizeof(attr_dis_hd));
  attr_dis_hd.uc_dev_idx = AUI_DIS_HD;

  if(0 != aui_find_dev_by_idx(AUI_MODULE_DIS, 0, &dis_hdl_hd)) {
    if(0 != aui_dis_open(&attr_dis_hd, &dis_hdl_hd)) {
      qWarning() << "skedplayer aui_dis_open fail";
      return;
    }
  }

  if(0 != aui_dis_video_enable(dis_hdl_hd, on)) {
    qWarning() << "skedplayer aui_dis_video_enable fail";
  }

  if(0 != aui_dis_close(&attr_dis_hd, &dis_hdl_hd)) {
    qWarning() << "skedplayer aui_dis_close fail";
  }
}

void SkedPlayer::onEnded()
{
  qDebug() << "skedplayer onEnded";
  enum STATE oldState = m_state;
  m_state = STATE_ENDED;
  emit stateChange(oldState, m_state);
}

void SkedPlayer::onStart()
{
  if (!m_inited) {
#if 0 // TODO
    if (! m_fullscreen) goplayer_set_display_rect(m_displayrect.left(), m_displayrect.top(), m_displayrect.width(), m_displayrect.height());
#endif
    displayEnableVideo(true);
    m_inited = true;
  }
  if (m_state == STATE_LOADED || m_state == STATE_PAUSED) {
    aui_mp_pause(m_mp_handle);
  }
}

void SkedPlayer::onBuffering(int percent)
{
  if (percent == 100 || m_state == STATE_PLAY) {
    emit buffering(percent);
  }
}

static void _callback(aui_mp_message type, void *data, void *userData)
{
  Q_UNUSED(userData);
  switch (type) {
  case AUI_MP_PLAY_BEGIN:
    qDebug() << "[callback] PLAY BEGIN";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "onStart", Qt::QueuedConnection);
    break;
  case AUI_MP_PLAY_END:
    qDebug() << "[callback] PLAY END";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "onEnded", Qt::QueuedConnection);
    break;
  case AUI_MP_BUFFERING: {
    int percent = (int)data;
    qDebug() << "[callback] buffering:" << percent;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "onBuffering", Qt::QueuedConnection, Q_ARG(int, percent));
  }
    break;
  case AUI_MP_VIDEO_CODEC_NOT_SUPPORT:
    qWarning() << "[callback] UNSUPPORT VIDEO";
    break;
  case AUI_MP_AUDIO_CODEC_NOT_SUPPORT:
    qWarning() << "[callback] UNSUPPORT AUDIO";
    break;
  case AUI_MP_RESOLUTION_NOT_SUPPORT:
    qWarning() << "[callback] RESOLUTION NOT SUPPORT";
    break;
  case AUI_MP_FRAMERATE_NOT_SUPPORT:
    qWarning() << "[callback] FRAMERATE NOT SUPPORT";
    break;
  case AUI_MP_NO_MEMORY:
    qWarning() << "[callback] NO MEMORY";
    break;
  case AUI_MP_DECODE_ERROR:
    qWarning() << "[callback] DECODE ERROR";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_DECODE));
    break;
  case AUI_MP_ERROR_UNKNOWN:
    qWarning() << "[callback] UNKNOW ERROR:" << (char *)data;
    break;
  case AUI_MP_ERROR_SOUPHTTP:
    qWarning() << "[callback] Soup http error, code:" << (int)data;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_NETWORK));
    break;
  default:
    break;
  }
}

static enum aui_mp_speed rateToAuiMpSpeed(double rate) {
  if (rate < -16) {
    qDebug() << "skedplayer" << "FR" << 24;
    return AUI_MP_SPEED_FASTREWIND_24;
  } else if (rate < -8) {
    qDebug() << "skedplayer" << "FR" << 16;
    return AUI_MP_SPEED_FASTREWIND_16;
  } else if (rate < -4) {
    qDebug() << "skedplayer" << "FR" << 8;
    return AUI_MP_SPEED_FASTREWIND_8;
  } else if (rate < -2) {
    qDebug() << "skedplayer" << "FR" << 4;
    return AUI_MP_SPEED_FASTREWIND_4;
  } else if (rate < 0) {
    qDebug() << "skedplayer" << "FR" << 2;
    return AUI_MP_SPEED_FASTREWIND_2;
  } else if (rate == 0) {
    return  AUI_MP_SPEED_0;
  } else if (rate <= 1) {
    return  AUI_MP_SPEED_1;
  } else if (rate > 1) {
    qDebug() << "skedplayer" << "FF" << 2;
    return AUI_MP_SPEED_FASTFORWARD_2;
  } else if (rate > 2) {
    qDebug() << "skedplayer" << "FF" << 4;
    return AUI_MP_SPEED_FASTFORWARD_4;
  } else if (rate > 4) {
    qDebug() << "skedplayer" << "FF" << 8;
    return AUI_MP_SPEED_FASTFORWARD_8;
  } else if (rate > 8) {
    qDebug() << "skedplayer" << "FF" << 16;
    return AUI_MP_SPEED_FASTFORWARD_16;
  } else if (rate > 16) {
    qDebug() << "skedplayer" << "FF" << 24;
    return AUI_MP_SPEED_FASTFORWARD_24;
  } else {
    qWarning() << "skedplayer" << "should not reach here";
    return  AUI_MP_SPEED_1;
  }
}
