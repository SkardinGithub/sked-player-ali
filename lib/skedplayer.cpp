#include "skedplayer.h"
#include <QtCore>
#include <stdlib.h>
extern "C" {
#include <aui_mp.h>
#include <aui_snd.h>
#include <aui_dis.h>
#include <aui_decv.h>
}

#define DISPLAY_AUTO_VIDEO_ON_OFF

SkedPlayer * SkedPlayer::m_instance = NULL;

static void _callback(aui_mp_message type, void *data, void *userData);
static enum aui_mp_speed rateToAuiMpSpeed(double rate);
static QString toAudioCodecName(int codec);
static QString toVideoCodecName(int codec);
static aui_hdl getAuiSoundHandle();
static aui_hdl getAuiDisplayHandle();

SkedPlayer::SkedPlayer(QObject *parent) : QObject(parent)
{
  qDebug() << "skedplayer constructor";
  if (SkedPlayer::m_instance)
    return;
  m_mp_handle = NULL;
  m_state = STATE_STOP;
  m_playback_rate = 1;
  m_buffer_level = 0;
  m_fullscreen = true;
  m_displayrect = QRect(0, 0, 1280, 720);
  setenv("GST_REGISTRY", "/tmp/gst_registry.bin", 1);
  setenv("OMX_BELLAGIO_REGISTRY", "/tmp/omx_bellagio_registry", 1);
  QProcess::execute("omxregister-bellagio", QStringList() << "/usr/lib/bellagio/");
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
  m_buffer_level = 0;
  m_fullscreen = true;
  m_audio_tracks.clear();
  m_video_tracks.clear();
  m_subtitle_tracks.clear();
  if (m_state != STATE_STOP) {
    stop_i();
  }
}

bool SkedPlayer::stop_i()
{
  qDebug() << "skedplayer stop_i";
  if (m_state != STATE_STOP) {
    saveStopTime();
    if (m_mp_handle) {
      /* TODO: Investigate resource management and correct switch from live view to player and back */
      /* walk around to recover live view after exit from player */
      aui_hdl decv_hdl = NULL;
      if (AUI_RTN_SUCCESS == aui_find_dev_by_idx(AUI_MODULE_DECV, 0, &decv_hdl))
      {
        aui_decv_start(decv_hdl);
        aui_decv_stop(decv_hdl);
      }
      aui_mp_close(NULL, &m_mp_handle);
      m_mp_handle = NULL;
    }
#ifdef DISPLAY_AUTO_VIDEO_ON_OFF
    displayAutoVideo(false);
#else
    displayEnableVideo(false);
    displayFillBlack();
#endif
    m_stop_elapsed_timer.restart();
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }

  return true;
}

bool SkedPlayer::stop()
{
  qDebug() << "skedplayer stop";
  m_playback_rate = 1;
  m_buffer_level = 0;
  m_fullscreen = true;
  return stop_i();
}

void SkedPlayer::load()
{
  qDebug() << "skedplayer load" << m_src << "from position" << m_start_time;
  if (m_state == STATE_LOADED) return;
  if (m_src.isEmpty()) return;
  m_buffer_level = 0;
  m_inited = false;
  m_duration = -1;
  if (m_state != STATE_STOP) {
    stop_i();
  }
  {
     qint64 elapsed = m_stop_elapsed_timer.elapsed();
     if (elapsed >= 0 && elapsed < 500) {
        QThread::msleep(500 - elapsed);
     }
  }
#ifdef DISPLAY_AUTO_VIDEO_ON_OFF
  displayAutoVideo(true);
#else
  displayFillBlack();
#endif
  soundSetOutMode();
  aui_attr_mp mp_attr;
  memset(&mp_attr, 0, sizeof(mp_attr));
  strncpy((char *)mp_attr.uc_file_name, qPrintable(m_src), 1024);
  mp_attr.stream_protocol = AUI_MP_STREAM_PROTOCOL_UNKNOW;
  mp_attr.start_time = m_start_time * 1000;
  mp_attr.aui_mp_stream_cb = _callback;
  mp_attr.user_data = NULL;
  aui_mp_open(&mp_attr, &m_mp_handle);
  aui_mp_start(m_mp_handle);
  m_start_time = 0;
  emit rateChange(m_playback_rate);
  emit displayRectChange(m_fullscreen, m_displayrect);
  enum STATE oldState = m_state;
  m_state = STATE_LOADED;
  emit stateChange(oldState, m_state);
  emit buffering(m_buffer_level);
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
    //m_buffer_level = 0;
    //emit buffering(m_buffer_level);
    break;
  case STATE_STOP:
    m_start_time = time;
    break;
  default:
    break;
  }
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

void SkedPlayer::setVolume(double vol)
{
  qDebug() << "skedplayer setVolume" << vol;

  aui_hdl hdl_snd = getAuiSoundHandle();
  if (hdl_snd == NULL) return;

  if (0 != aui_snd_vol_set(hdl_snd, vol * 100 + 0.5)) {
    qWarning() << "skedplayer aui_snd_vol_set fail";
    return;
  }
  unsigned char mute;
  aui_snd_mute_get(hdl_snd, &mute);
  emit volumeChange(mute, vol);
}

double SkedPlayer::getVolume()
{
  aui_hdl hdl_snd = getAuiSoundHandle();
  if (hdl_snd == NULL) return 0;
  unsigned char vol = 0;
  if (0 != aui_snd_vol_get(hdl_snd, &vol)) {
    qWarning() << "skedplayer aui_snd_vol_get fail";
  }
  return vol / 100.0;
}

void SkedPlayer::mute(bool mute)
{
  qDebug() << "skedplayer " << (mute ? "mute" : "unmute");
  aui_hdl hdl_snd = getAuiSoundHandle();
  if (hdl_snd == NULL) return;
  if (0 != aui_snd_mute_set(hdl_snd, mute)) {
    qWarning() << "skedplayer aui_snd_mute_set fail";
    return;
  }
  unsigned char vol;
  aui_snd_vol_get(hdl_snd, &vol);
  emit volumeChange(mute, vol/100.0);
}

bool SkedPlayer::muted()
{
  aui_hdl hdl_snd = getAuiSoundHandle();
  if (hdl_snd == NULL) return true;
  unsigned char mute = 1;
  if (0 != aui_snd_mute_get(hdl_snd, &mute)) {
    qWarning() << "skedplayer aui_snd_mute_get fail";
  }
  return (mute != 0);
}

void SkedPlayer::setDisplayRect(const QRect & rect)
{
  qDebug() << "skedplayer set display rect" << rect;
  m_displayrect = rect;
  // TODO
  if (! m_fullscreen && m_state != STATE_STOP)
    aui_mp_set_display_rect(m_mp_handle, rect.left(), rect.top(), rect.width(), rect.height());
  emit displayRectChange(m_fullscreen, m_displayrect);
}

void SkedPlayer::setFullScreen(bool full)
{
  qDebug() << "skedplayer" << "fullscreen" << full;
  m_fullscreen = full;
  QRect rect = (m_fullscreen ? QRect(0, 0, 1280, 720) : m_displayrect);
  // TODO
  if (m_state != STATE_STOP)
    aui_mp_set_display_rect(m_mp_handle, rect.left(), rect.top(), rect.width(), rect.height());
  emit displayRectChange(m_fullscreen, m_displayrect);
}

void SkedPlayer::displayFillBlack()
{
  qDebug() << "skedplayer display fill black";
  aui_hdl dis_hdl = getAuiDisplayHandle();
  if (dis_hdl == NULL) return;
  if(0 != aui_dis_fill_black_screen(dis_hdl)) {
    qWarning() << "skedplayer aui_dis_fill_black_screen fail";
  }
}

void SkedPlayer::displayEnableVideo(bool on)
{
  qDebug() << "skedplayer" << (on ? "enable" : "disable") << "display";
  aui_hdl dis_hdl = getAuiDisplayHandle();
  if (dis_hdl == NULL) return;
  if(0 != aui_dis_video_enable(dis_hdl, on)) {
    qWarning() << "skedplayer aui_dis_video_enable fail";
  }
}

void SkedPlayer::displayAutoVideo(bool on)
{
  qDebug() << "skedplayer" << (on ? "enable" : "disable") << "auto video display";
  aui_hdl dis_hdl = getAuiDisplayHandle();
  if (dis_hdl == NULL) return;
  quint32 disable = !on;
  if (0 != aui_dis_set(dis_hdl, AUI_DIS_SET_AUTO_WINONOFF, &disable)) {
    qWarning() << "skedplayer aui_dis_set AUI_DIS_SET_AUTO_WINONOFF fail";
  }
}

void SkedPlayer::soundSetOutMode()
{
  aui_snd_out_mode out_mode;
  memset(&out_mode, 0, sizeof(aui_snd_out_mode));
  //out_mode.snd_data_type = AUI_SND_OUT_MODE_DECODED;
  //out_mode.snd_data_type = AUI_SND_OUT_MODE_FORCE_DD;
  out_mode.snd_data_type = AUI_SND_OUT_MODE_ENCODED;
  out_mode.snd_out_type = AUI_SND_OUT_HDMI;

  aui_hdl snd = getAuiSoundHandle();
  if (AUI_RTN_SUCCESS != aui_snd_out_data_type_set(snd, out_mode)) {
    qWarning() << "aui_snd_out_data_type_set() failed";
  } else {
    qDebug() << "set sound out mode to" <<
              ((AUI_SND_OUT_MODE_ENCODED == out_mode.snd_data_type) ? "DD+" :
              ((AUI_SND_OUT_MODE_FORCE_DD == out_mode.snd_data_type) ? "DD" : "PCM"));
  }
}

QVariantList SkedPlayer::videoTracks()
{
  if (m_state == STATE_STOP)
    return QVariantList();

  if (!m_video_tracks.isEmpty())
    return m_video_tracks;

  aui_mp_stream_info *stream_info;
  if (AUI_RTN_SUCCESS == aui_mp_get_stream_info(m_mp_handle, AUI_MP_STREAM_INFO_TYPE_VIDEO, &stream_info)) {
    for (unsigned int i = 0; i < stream_info->count; i++) {
      aui_mp_video_track_info *track_info = &stream_info->stream_info.video_track_info[i];
      QVariantMap track;
      track["id"] = int(0);
      track["width"] = int(track_info->width);
      track["height"] = int(track_info->height);
      track["framerate"] = double(track_info->framerate) / 1000;
      track["format"] = toVideoCodecName(track_info->vidCodecFmt);
      track["enable"] = true;
      m_video_tracks.append(track);
    }
    aui_mp_free_stream_info(m_mp_handle, stream_info);
  }

  return m_video_tracks;
}

QVariantList SkedPlayer::audioTracks()
{
  if (m_state == STATE_STOP)
    return QVariantList();

  if (!m_audio_tracks.isEmpty())
    return m_audio_tracks;

  aui_mp_stream_info *stream_info = NULL;
  aui_mp_stream_info *cur_stream_info = NULL;

  if (AUI_RTN_SUCCESS != aui_mp_get_cur_stream_info(m_mp_handle, AUI_MP_STREAM_INFO_TYPE_AUDIO, &cur_stream_info)) {
    return QVariantList();
  }
  if (AUI_RTN_SUCCESS != aui_mp_get_stream_info(m_mp_handle, AUI_MP_STREAM_INFO_TYPE_AUDIO, &stream_info)) {
    aui_mp_free_stream_info(m_mp_handle, cur_stream_info);
    return QVariantList();
  }

  aui_mp_audio_track_info *cur_track_info = &cur_stream_info->stream_info.audio_track_info[0];

  for (unsigned int i = 0; i < stream_info->count; i++) {
    aui_mp_audio_track_info *track_info = &stream_info->stream_info.audio_track_info[i];
    QVariantMap track;
    track["id"] = int(track_info->track_index);
    track["lang"] = QString::fromLatin1(track_info->lang_code, 3);
    if (track_info->audDetailInfo) {
      track["format"] = toAudioCodecName(track_info->audDetailInfo->audioCodecType);
      track["channels"] = int(track_info->audDetailInfo->channels);
    }
    if (track_info->track_index == cur_track_info->track_index) {
      track["enable"] = true;
      track["lang"] = QString::fromLatin1(cur_track_info->lang_code, 3);
      if (cur_track_info->audDetailInfo) {
        track["format"] = toAudioCodecName(cur_track_info->audDetailInfo->audioCodecType);
        track["channels"] = int(cur_track_info->audDetailInfo->channels);
      }
    } else {
      track["enable"] = false;
    }
    m_audio_tracks.append(track);
  }

  aui_mp_free_stream_info(m_mp_handle, cur_stream_info);
  aui_mp_free_stream_info(m_mp_handle, stream_info);

  return m_audio_tracks;
}

QVariantList SkedPlayer::subtitleTracks()
{
  if (m_state == STATE_STOP)
    return QVariantList();

  if (!m_subtitle_tracks.isEmpty())
    return m_subtitle_tracks;

  aui_mp_stream_info *stream_info = NULL;
  aui_mp_stream_info *cur_stream_info = NULL;

  if (AUI_RTN_SUCCESS != aui_mp_get_cur_stream_info(m_mp_handle, AUI_MP_STREAM_INFO_TYPE_SUBTITLE, &cur_stream_info)) {
    return QVariantList();
  }
  if (AUI_RTN_SUCCESS != aui_mp_get_stream_info(m_mp_handle, AUI_MP_STREAM_INFO_TYPE_SUBTITLE, &stream_info)) {
    aui_mp_free_stream_info(m_mp_handle, cur_stream_info);
    return QVariantList();
  }

  aui_mp_subtitle_info *cur_track_info = &cur_stream_info->stream_info.subtitle_info[0];

  for (unsigned int i = 0; i < stream_info->count; i++) {
    aui_mp_subtitle_info *track_info = &stream_info->stream_info.subtitle_info[i];
    QVariantMap track;
    track["id"] = int(track_info->track_index);
    track["lang"] = QString::fromLatin1(track_info->lang_code, 3);
    if (track_info->track_index == cur_track_info->track_index) {
      track["enable"] = true;
      track["lang"] = QString::fromLatin1(cur_track_info->lang_code, 3);
    } else {
      track["enable"] = false;
    }
    m_subtitle_tracks.append(track);
  }

  aui_mp_free_stream_info(m_mp_handle, cur_stream_info);
  aui_mp_free_stream_info(m_mp_handle, stream_info);

  return m_subtitle_tracks;
}

bool SkedPlayer::setCurrentAudioTrack(int track_index)
{
  if (m_state == STATE_STOP)
    return false;
  m_audio_tracks.clear();
  return (AUI_RTN_SUCCESS == aui_mp_change_audio(m_mp_handle, track_index));
}

bool SkedPlayer::setCurrentSubtitleTrack(int track_index)
{
  if (m_state == STATE_STOP)
    return false;
  m_subtitle_tracks.clear();
  return (AUI_RTN_SUCCESS == aui_mp_change_subtitle(m_mp_handle, track_index));
}

void SkedPlayer::onEnded()
{
  qDebug() << "skedplayer onEnded";
  clearStopTime();
  enum STATE oldState = m_state;
  m_state = STATE_ENDED;
  emit stateChange(oldState, m_state);
  if (m_buffer_level != 100) {
    m_buffer_level = 100;
    emit buffering(m_buffer_level);
  }
}

void SkedPlayer::onStart()
{
  if (!m_inited) {
    if (! m_fullscreen) {
      aui_mp_set_display_rect(m_mp_handle, m_displayrect.left(), m_displayrect.top(), m_displayrect.width(), m_displayrect.height());
    }
#ifndef DISPLAY_AUTO_VIDEO_ON_OFF
    displayEnableVideo(true);
#endif
    if (m_buffer_level != 100) {
      m_buffer_level = 100;
      emit buffering(m_buffer_level);
    }
    m_inited = true;
  }
  if (m_state == STATE_LOADED || m_state == STATE_PAUSED) {
    aui_mp_pause(m_mp_handle);
  }
}

void SkedPlayer::onBuffering(int percent)
{
  m_buffer_level = percent;
  emit buffering(m_buffer_level);
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
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_DECODE));
    break;
  case AUI_MP_AUDIO_CODEC_NOT_SUPPORT:
    qWarning() << "[callback] UNSUPPORT AUDIO";
    break;
  case AUI_MP_RESOLUTION_NOT_SUPPORT:
    qWarning() << "[callback] RESOLUTION NOT SUPPORT";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_DECODE));
    break;
  case AUI_MP_FRAMERATE_NOT_SUPPORT:
    qWarning() << "[callback] FRAMERATE NOT SUPPORT";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_DECODE));
    break;
  case AUI_MP_NO_MEMORY:
    qWarning() << "[callback] NO MEMORY";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_SYSTEM));
    break;
  case AUI_MP_DECODE_ERROR:
    qWarning() << "[callback] DECODE ERROR";
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_DECODE));
    break;
  case AUI_MP_ERROR_UNKNOWN:
    qWarning() << "[callback] UNKNOW ERROR:" << (char *)data;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_SYSTEM));
    break;
  case AUI_MP_ERROR_SOUPHTTP:
    qWarning() << "[callback] Soup http error, code:" << (int)data;
    QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_NETWORK));
    break;
  default:
    break;
  }
}

static aui_hdl getAuiSoundHandle()
{
  aui_hdl hdl_snd;
  if (0 != aui_find_dev_by_idx(AUI_MODULE_SND, 0, &hdl_snd)) {
    aui_attr_snd attr_snd;
    memset(&attr_snd, 0, sizeof(aui_attr_snd));
    if (0 != aui_snd_open(&attr_snd, &hdl_snd)) {
      qWarning() << "skedplayer aui_snd_open fail";
      return NULL;
    }
  }

  return hdl_snd;
}

static aui_hdl getAuiDisplayHandle()
{
  aui_hdl dis_hdl_hd;

  if (0 != aui_find_dev_by_idx(AUI_MODULE_DIS, 0, &dis_hdl_hd)) {
    aui_attr_dis attr_dis_hd;
    memset(&attr_dis_hd, 0, sizeof(attr_dis_hd));
    attr_dis_hd.uc_dev_idx = AUI_DIS_HD;
    if (0 != aui_dis_open(&attr_dis_hd, &dis_hdl_hd)) {
      qWarning() << "skedplayer aui_dis_open fail";
      return NULL;
    }
  }

  return dis_hdl_hd;
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
    qDebug() << "skedplayer speed 0";
    return  AUI_MP_SPEED_0;
  } else if (rate < 2) {
    qDebug() << "skedplayer speed 1";
    return  AUI_MP_SPEED_1;
  } else if (rate < 4) {
    qDebug() << "skedplayer" << "FF" << 2;
    return AUI_MP_SPEED_FASTFORWARD_2;
  } else if (rate < 8) {
    qDebug() << "skedplayer" << "FF" << 4;
    return AUI_MP_SPEED_FASTFORWARD_4;
  } else if (rate < 16) {
    qDebug() << "skedplayer" << "FF" << 8;
    return AUI_MP_SPEED_FASTFORWARD_8;
  } else if (rate < 24) {
    qDebug() << "skedplayer" << "FF" << 16;
    return AUI_MP_SPEED_FASTFORWARD_16;
  } else {
    qDebug() << "skedplayer" << "FF" << 24;
    return AUI_MP_SPEED_FASTFORWARD_24;
  }
}

static QString toAudioCodecName(int codec)
{
  switch (codec) {
  case AUI_DECA_STREAM_TYPE_MPEG1:
    return "MPEG1";
  case AUI_DECA_STREAM_TYPE_MPEG2:
    return "MPEG2";
  case AUI_DECA_STREAM_TYPE_AAC_LATM:
    return "AAC";
  case AUI_DECA_STREAM_TYPE_AC3:
    return "AC3";
  case AUI_DECA_STREAM_TYPE_DTS:
    return "DTA";
  case AUI_DECA_STREAM_TYPE_PPCM:
    return "PCM";
  case AUI_DECA_STREAM_TYPE_LPCM_V:
    return "LPCM";
  case AUI_DECA_STREAM_TYPE_LPCM_A:
    return "LPCM";
  case AUI_DECA_STREAM_TYPE_PCM:
    return "PCM";
  case AUI_DECA_STREAM_TYPE_BYE1:
    return "BYE1";
  case AUI_DECA_STREAM_TYPE_RA8:
    return "RA8";
  case AUI_DECA_STREAM_TYPE_MP3:
    return "MP3";
  case AUI_DECA_STREAM_TYPE_AAC_ADTS:
    return "AAC";
  case AUI_DECA_STREAM_TYPE_OGG:
    return "OGG";
  case AUI_DECA_STREAM_TYPE_EC3:
    return "EC3";
  case AUI_DECA_STREAM_TYPE_MP3_L3:
    return "MP3";
  case AUI_DECA_STREAM_TYPE_RAW_PCM:
    return "PCM";
  case AUI_DECA_STREAM_TYPE_BYE1PRO:
    return "BYE1PRO";
  case AUI_DECA_STREAM_TYPE_FLAC:
    return "FLAC";
  case AUI_DECA_STREAM_TYPE_APE:
    return "APE";
  case AUI_DECA_STREAM_TYPE_MP3_2:
    return "MP3";
  case AUI_DECA_STREAM_TYPE_AMR:
    return "AMR";
  case AUI_DECA_STREAM_TYPE_ADPCM:
    return "ADPCM";
  default:
    return "UNKNOWN";
  }

  return "UNKNOWN";
}

static QString toVideoCodecName(int codec)
{
  switch (codec) {
  case AUI_DECV_FORMAT_MPEG:
    return "MPEG";
  case AUI_DECV_FORMAT_AVC:
    return "H264";
  case AUI_DECV_FORMAT_AVS:
    return "AVS";
  case AUI_DECV_FORMAT_XVID:
    return "XVID";
  case AUI_DECV_FORMAT_FLV1:
    return "FLV";
  case AUI_DECV_FORMAT_VP8:
    return "VP8";
  case AUI_DECV_FORMAT_WVC1:
    return "WVC1";
  case AUI_DECV_FORMAT_WX3:
    return "WX3";
  case AUI_DECV_FORMAT_RMVB:
    return "RMVB";
  case AUI_DECV_FORMAT_MJPG:
    return "MJPEG";
  case AUI_DECV_FORMAT_HEVC:
    return "H265";
  default:
    return "UNKNOWN";
  }

  return "UNKNOWN";
}

static QString getBookMarkFileName(const QString &src)
{
  if (!src.startsWith("file://")) return QString();
  return QFileInfo(src.mid(7)).absoluteFilePath() + ".skbm";
}

static QJsonObject getBookMarkJson(const QString &path)
{
  QJsonDocument jd;
  QFile file(path);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    jd = QJsonDocument::fromJson(file.readAll());
    file.close();
  }
  return jd.object();
}

static void saveStopTimeToBookMarkFile(const QString &path, double time)
{
  QJsonObject o = getBookMarkJson(path);
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    qWarning() << "skedplayer can not open" << path;
    return;
  }
  o["last_stop_time"] = time;
  file.write(QJsonDocument(o).toJson());
  file.close();
}

static double getStopTimeFromBookMarkFile(const QString &path)
{
  QJsonObject o = getBookMarkJson(path);
  if (o.contains("last_stop_time")) {
    return o["last_stop_time"].toDouble();
  }
  return 0;
}

void SkedPlayer::clearStopTime()
{
  QString path = getBookMarkFileName(m_src);
  if (path.isEmpty()) return;
  qDebug() << "skedplayer clear stop time for" << m_src;
  saveStopTimeToBookMarkFile(path, 0);
}

void SkedPlayer::saveStopTime()
{
  if (m_state != STATE_PAUSED && m_state != STATE_PLAY) return;

  QString path = getBookMarkFileName(m_src);
  if (path.isEmpty()) return;

  double current_time = getCurrentTime();
  double duration = this->duration();
  if (current_time + 5 > duration) {
    qDebug() << "skedplayer clearing stop time @" << current_time << "/" << duration;
    clearStopTime();
    return;
  }

  qDebug() << "skedplayer saving stop time @" << current_time << "/" << duration;
  saveStopTimeToBookMarkFile(path, current_time);
}

double SkedPlayer::getLastStopTime(const QString &src)
{
  QString path = getBookMarkFileName(src);
  if (path.isEmpty()) return 0;
  return getStopTimeFromBookMarkFile(path);
}
