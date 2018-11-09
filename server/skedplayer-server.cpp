#include "skedplayer.h"
#include <QtCore>
#include <gst/gst.h>

SkedPlayer * SkedPlayer::m_instance = NULL;

static gboolean bus_callback (GstBus *bus, GstMessage *msg, gpointer data);

SkedPlayer::SkedPlayer(QObject *parent) : QObject(parent)
{
  qDebug() << "skedplayer constructor";
  if (SkedPlayer::m_instance)
    return;
  m_state = STATE_STOP;
  m_playback_rate = 1;
  m_buffer_level = 0;
  m_fullscreen = true;
  m_displayrect = QRect(0, 0, 1280, 720);
  m_vol = 0.5;
  m_mute = false;
  if (!gst_is_initialized()) {
    gst_init(NULL, NULL);
  }
  SkedPlayer::m_instance = this;
}

SkedPlayer::~SkedPlayer()
{
  qDebug() << "skedplayer destructor";
}

void SkedPlayer::setSrc(const QString &src)
{
  qDebug() << "skedplayer set source" << src;
  m_src = src;
  m_start_time = 0;
  m_playback_rate = 1;
  m_buffer_level = 0;
  m_fullscreen = true;
  if (m_playbin) {
    gst_element_set_state(m_playbin, GST_STATE_NULL);
    gst_object_unref(m_playbin);
    m_playbin = NULL;
  }
  if (m_state != STATE_STOP) {
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }
}

void SkedPlayer::stop()
{
  qDebug() << "skedplayer stop";
  m_playback_rate = 1;
  m_buffer_level = 0;
  m_fullscreen = true;
  if (m_playbin) {
    gst_element_set_state(m_playbin, GST_STATE_NULL);
    gst_object_unref(m_playbin);
    m_playbin = NULL;
  }
  if (m_state != STATE_STOP) {
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }
}

void SkedPlayer::load()
{
  qDebug() << "+skedplayer load" << m_src << "from position" << m_start_time;
  if (m_state == STATE_LOADED) return;
  if (m_src.isEmpty()) return;
  m_buffer_level = 0;
  m_inited = false;
  m_duration = -1;
  if (m_playbin) {
    gst_element_set_state(m_playbin, GST_STATE_NULL);
    gst_object_unref(m_playbin);
    m_playbin = NULL;
  }
  if (m_state != STATE_STOP) {
    enum STATE oldState = m_state;
    m_state = STATE_STOP;
    emit stateChange(oldState, m_state);
  }
  m_playbin = gst_element_factory_make("playbin", NULL);
  if (!m_playbin) {
    qCritical("skedplayer Failed to create playbin2");
    return;
  }
  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(m_playbin));
  gst_bus_add_watch (bus, bus_callback, NULL);
  gst_object_unref(bus);
  gst_element_set_state (m_playbin, GST_STATE_NULL);
  g_object_set(G_OBJECT(m_playbin), "uri", qPrintable(m_src), NULL);
  gst_element_set_state(m_playbin, GST_STATE_PAUSED);
  m_start_time = 0;
  emit rateChange(m_playback_rate);
  emit displayRectChange(m_fullscreen, m_displayrect);
  enum STATE oldState = m_state;
  m_state = STATE_LOADED;
  emit stateChange(oldState, m_state);
  m_buffer_level = 100;
  emit buffering(m_buffer_level);
}

void SkedPlayer::play()
{
  qDebug() << "+skedplayer play";
  if (m_state == STATE_PLAY) return;
  if (m_src.isEmpty()) return;
  if (m_state == STATE_STOP || m_state == STATE_ENDED) load();
  gst_element_set_state(m_playbin, GST_STATE_PLAYING);
  enum STATE oldState = m_state;
  m_state = STATE_PLAY;
  emit stateChange(oldState, m_state);
}

void SkedPlayer::pause()
{
  qDebug() << "skedplayer pause";
  if (m_state == STATE_LOADED || m_state == STATE_PLAY) {
    gst_element_set_state(m_playbin, GST_STATE_PAUSED);
    enum STATE oldState = m_state;
    m_state = STATE_PAUSED;
    emit stateChange(oldState, m_state);
  }
}

double SkedPlayer::getCurrentTime()
{
  if (m_state == STATE_STOP) return -1;
  if (m_state == STATE_ENDED) return m_duration;
  gint64 position;
  if (gst_element_query_position (m_playbin, GST_FORMAT_TIME, &position)) {
    return GST_TIME_AS_MSECONDS(position) / 1000.0;
  }
  return -1;
}

bool SkedPlayer::seekable()
{
  if (m_state == STATE_STOP) return false;
  return true;
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
    gst_element_seek(m_playbin, m_playback_rate, GST_FORMAT_TIME,
                     (GstSeekFlags) ((int) GST_SEEK_FLAG_FLUSH | (int) GST_SEEK_FLAG_KEY_UNIT),
                     GST_SEEK_TYPE_SET, time * GST_SECOND, GST_SEEK_TYPE_NONE, 0);
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
  gint64 duration;
  if (gst_element_query_duration (m_playbin, GST_FORMAT_TIME, &duration)) {
    m_duration = double(duration) / GST_SECOND;
  }
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
  setCurrentTime(getCurrentTime());
  emit rateChange(m_playback_rate);
}

void SkedPlayer::setVolume(double vol)
{
  qDebug() << "skedplayer setVolume" << vol;
  if (vol >= 0 and vol <= 1.0) {
    m_vol = vol;
    g_object_set(m_playbin, "volume", m_vol, NULL);
    emit volumeChange(m_mute, m_vol);
  }
}

double SkedPlayer::getVolume()
{
  return m_vol;
}

void SkedPlayer::mute(bool mute)
{
  qDebug() << "skedplayer " << (mute ? "mute" : "unmute");
  m_mute = mute;
  g_object_set(m_playbin, "volume", m_mute ? 0 : m_vol, NULL);
  emit volumeChange(m_mute, m_vol);
}

bool SkedPlayer::muted()
{
  return m_mute;
}

void SkedPlayer::setDisplayRect(const QRect & rect)
{
  qDebug() << "skedplayer set display rect" << rect;
  // TODO
  m_displayrect = rect;
  emit displayRectChange(m_fullscreen, m_displayrect);
}

void SkedPlayer::setFullScreen(bool full)
{
  qDebug() << "skedplayer" << "fullscreen" << full;
  m_fullscreen = full;
  //QRect rect = (m_fullscreen ? QRect(0, 0, 1280, 720) : m_displayrect);
  // TODO
  emit displayRectChange(m_fullscreen, m_displayrect);
}

void SkedPlayer::onEnded()
{
  if (m_mp_handle)
  {
      /* TODO: Investigate resource management and correct switch from live view to player and back */
      /* walk around to recover live view after exit from player */
      aui_hdl decv_hdl = NULL;
      if (AUI_RTN_SUCCESS == aui_find_dev_by_idx(AUI_MODULE_DECV, 0, &decv_hdl))
      {
        aui_decv_start(decv_hdl);
        aui_decv_stop(decv_hdl);
        qDebug() << "start/stop";
      }
      aui_mp_close(NULL, &m_mp_handle);
      m_mp_handle = NULL;
  }
//  displayFillBlack();
  enum STATE oldState = m_state;
  m_state = STATE_ENDED;
  emit stateChange(oldState, m_state);
  qDebug() << "SHKEDplayer onEnded";
  if (m_buffer_level != 100) {
    m_buffer_level = 100;
    emit buffering(m_buffer_level);
  }
}

void SkedPlayer::onStart()
{
}

void SkedPlayer::onBuffering(int percent)
{
  m_buffer_level = percent;
  emit buffering(m_buffer_level);
}

static gboolean bus_callback (GstBus *bus, GstMessage *msg, gpointer data)
{
  Q_UNUSED(bus);
  Q_UNUSED(data);

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      QMetaObject::invokeMethod(SkedPlayer::singleton(), "onEnded", Qt::QueuedConnection);
      break;
    case GST_MESSAGE_WARNING: {
      gchar *debug;
      GError *err;
      gst_message_parse_warning(msg, &err, &debug);
      qWarning("%s: %s", GST_MESSAGE_SRC_NAME (msg), err->message);
      g_error_free(err);
      if (debug) {
        qWarning("Debug details: %s", debug);
        g_free (debug);
      }
      break;
    }
    case GST_MESSAGE_ERROR: {
      gchar *debug = NULL;
      GError *err = NULL;
      gst_message_parse_error (msg, &err, &debug);
      qWarning("%s : %s", GST_MESSAGE_SRC_NAME (msg), err->message);
      g_error_free (err);
      if (debug) {
        qWarning("Debug details: %s", debug);
        g_free (debug);
      }
      QMetaObject::invokeMethod(SkedPlayer::singleton(), "error", Qt::QueuedConnection, Q_ARG(int, SkedPlayer::ERROR_NETWORK));
      break;
    }
    case GST_MESSAGE_BUFFERING: {
      gint percent;
      gst_message_parse_buffering(msg, &percent);
      qDebug() << GST_MESSAGE_SRC_NAME (msg) << "buffering" << percent;
      QMetaObject::invokeMethod(SkedPlayer::singleton(), "onBuffering", Qt::QueuedConnection, Q_ARG(int, percent));
      break;
    default:
      break;
    }
  }

  return TRUE;
}
