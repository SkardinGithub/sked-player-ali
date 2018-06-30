import QtQuick 2.0
import SKED.MediaPlayer 1.0

Rectangle {
  id: root
  width: 1280
  height: 720
  color: "#00000000"
  focus: true
  property int channel: 0

  function sources() {
    return [
      ["MP4/H264/AAC", "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"],
      ["HLS LIVE", "http://static.france24.com/live/F24_EN_LO_HLS/live_web.m3u8"],
      ["HLS VOD", "http://modavpkg1.eng.cloud.bbc.co.uk/usp/vod/iplayer_vod/int/ism_int/iptv_hd_abr_hls_v1.ism/.m3u8"],
      ["Smooth Streaming", "http://playready.directtaps.net/smoothstreaming/SSWSS720H264/SuperSpeedway_720.ism/Manifest"],
      ["Radio", "http://stream.lounge.fm/mp3/"],
      ["USB MP3", "file:///mnt/usb/sda1/test.mp3"],
      ["USB 2xAud 2xSub", "file:///mnt/usb/sda1/1mp4_2aac_2sub.mp4"],
      ["MP4/H264/AAC", "http://192.168.1.226:8000/m/v/mp4/gstreamer/full_hd.mp4"],
      ["HLS VOD", "http://192.168.1.226:8000/m/hls/clear/bbc/iplayer/poldark/master.m3u8"],
      ["Smooth Streaming", "http://192.168.1.226:8000/m/sstr/clear/ssw_720p_h264/Manifest"],
    ];
  }

  function playChannel(id) {
    var srcs = sources();
    if (id >= srcs.length) return;
    root.channel = id;
    channel.text = id;
    channelList.visible = false;
    channelTimer.restart();
  }

  Timer {
    id: channelTimer
    interval: 100; running: false; repeat: false;
    onTriggered: {
      console.log('[mediaplayer] play channel ' + root.channel);
      var srcs = sources();
      if (root.channel < srcs.length) {
        Player.stop();
        buffer.text = ""
        progress.text = ""
        rate.text = ""
        error.text = ""
        Player.src = srcs[root.channel][1];
        //Player.currentTime = 30.0;
        //Player.displayrect = Qt.rect(320, 180, 640, 360);
        Player.load();
        Player.play();
      }
    }
  }

  Timer {
    id: progressTimer
    interval: 1000; running: false; repeat: true
    onTriggered: {
      var state = Player.state;
      if (state === Player.STATE_STOP) {
        progress.text = ''
      } else {
        var currentTime = Player.currentTime;
        if (Player.currentTime >= 0)
          progress.text = Math.round(currentTime) + '/' + Math.max(Math.round(Player.duration), 0)
      }
    }
  }

  Component.onCompleted: {
    //playChannel(0);
  }

  Rectangle {
    color: "#C0808080"
    y: 20
    x: 1180
    width: 80
    height: 60

    Text { color: "white"; font.bold: true; font.pointSize: 32;
      anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; margins: 20 }
      id: channel
      text: "--"
    }
  }

  Rectangle {
    id: channelList
    color: "#C0808080"
    anchors { horizontalCenter: parent.horizontalCenter; }
    y: 100
    width: parent.width - 40
    height: 500

    ListView {
      id : sourceView
      anchors { fill: parent; margins: 20; }
      model: ListModel {
        id : sourceModel
        Component.onCompleted: {
          var srcs = sources();
          for (var i =0; i < srcs.length; i++) {
            var src = srcs[i];
            sourceModel.append({index: i, name: src[0], url: src[1]})
          }
        }
      }
      delegate: Row {
        width: parent.width
        Text { text: "[" + index + "]"; width: 40; color: "white"; font.bold: true; font.pointSize: 24; }
        Text { text: name; width: 240; color: "white"; font.bold: true; font.pointSize: 24; }
        Text { text: url; width: parent.width - 280; color: "white"; font.bold: true; font.pointSize: 24; wrapMode: Text.WrapAnywhere; }
      }
    }
  }

  Rectangle {
    color: "#C0808080"
    y: 620
    anchors { horizontalCenter: parent.horizontalCenter; }
    width: parent.width - 40
    height: 60

    Row {
      id: control
      anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; }
      width: parent.width - 40

      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: state
        text: {
          if (Player.state === Player.STATE_STOP) return "Stop";
          else if (Player.state === Player.STATE_ENDED) return "Ended";
          else if (Player.state === Player.STATE_LOADED) return "Loaded";
          else if (Player.state === Player.STATE_PAUSED) return "Paused";
          else if (Player.state === Player.STATE_PLAY) return "Play";
          else return "";
        }
        width: 100
      }
      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: buffer
        text: ""
        width: 250
      }
      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: progress
        text: ""
        width: 200
      }
      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: rate
        text: (Player.rate === 1 ? '' : Player.rate + 'X')
        width: 100
      }
      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: volume
        text: "vol: " + Player.vol
        width: 100
      }
      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: mute
        text: (Player.mute ? "mute" : "")
        width: 100
      }
      Text { color: "white"; font.bold: true; font.pointSize: 24;
        id: error;
        text: ""
        width: 100
      }
    }
  }

  Keys.onPressed: {
    console.log('[mediaplayer] key pressed. code: ' + event.key)
    switch (event.key) {
    case Qt.Key_Return:
      if (!channelList.visible) channelList.visible = true;
      else if (Player.state !== Player.STATE_STOP) channelList.visible = false;
      break;
    case Qt.Key_Stop:
      console.log('[mediaplayer] close')
      Player.stop()
      break;
    case Qt.Key_MediaPlay: // RCU PlAY_PAUSE
      console.log('[mediaplayer] play_pause')
      if (Player.state === Player.STATE_PLAY) Player.pause();
      else Player.play();
      break;
    case Qt.Key_Right:
      console.log('[mediaplayer] seek +10')
      if (Player.seekable && Player.currentTime < Player.duration - 10) Player.currentTime += 10.0;
      break;
    case Qt.Key_Left:
      console.log('[mediaplayer] seek -10')
      if (Player.seekable && Player.currentTime > 10) Player.currentTime -= 10.0;
      break;
    case Qt.Key_VolumeUp:
      console.log('[mediaplayer] volume up')
      Player.vol = Math.min(Player.vol + 0.1, 1.0);
      break;
    case Qt.Key_VolumeDown:
      console.log('[mediaplayer] volume down')
      Player.vol = Math.max(Player.vol - 0.1, 0.0);
      break;
    case Qt.Key_N: // RCU MUTE
      console.log('[mediaplayer] mute/unmute')
      Player.mute = !Player.mute
      break;
    case Qt.Key_MediaPrevious: // RCU FRW
      switch (Player.rate) {
        case 1: Player.rate = -2; break;
        case 2: Player.rate = 1; break;
        default: Player.rate -= 2;
      }
      console.log('[mediaplayer] FRW %sX', Player.rate)
      break;
    case Qt.Key_MediaNext: // RCU FFW
      switch (Player.rate) {
        case -2: Player.rate = 1; break;
        case 1: Player.rate = 2; break;
        default: Player.rate += 2;
      }
      console.log('[mediaplayer] FFW %sX', Player.rate)
      break;
    case Qt.Key_Down:
      console.log('[mediaplayer] zoom in')
      var displayRect = Player.displayrect;
      Player.fullscreen = false;
      Player.displayrect = Qt.rect(displayRect.x + displayRect.width / 20, displayRect.y + displayRect.height / 20, displayRect.width * 0.9, displayRect.height * 0.9)
      break;
    case Qt.Key_Up:
      console.log('[mediaplayer] zoom out')
      var displayRect = Player.displayrect;
      Player.fullscreen = false;
      Player.displayrect = Qt.rect(displayRect.x - displayRect.width / 20, displayRect.y - displayRect.height / 20, displayRect.width * 1.1, displayRect.height * 1.1)
      break;
    case Qt.Key_0:
    case Qt.Key_1:
    case Qt.Key_2:
    case Qt.Key_3:
    case Qt.Key_4:
    case Qt.Key_5:
    case Qt.Key_6:
    case Qt.Key_7:
    case Qt.Key_8:
    case Qt.Key_9:
      var ch = event.key - 48
      if (ch < sources().length) playChannel(ch);
      break;
    case Qt.Key_ChannelUp:
      console.log('[mediaplayer] Ch+')
      if (root.channel < sources().length - 1) playChannel(root.channel + 1);
      break;
    case Qt.Key_ChannelDown:
      console.log('[mediaplayer] Ch-')
      if (root.channel > 0) playChannel(root.channel - 1);
      break;
    default:
      break;
    }
  }

  Connections {
    target: Player

    onStateChange: {
      console.log('[mediaplayer] event statechange ' + Player.state)
      switch (Player.state) {
      case Player.STATE_LOADED:
        rate.text = (Player.rate === 1 ? '' : Player.rate + 'X')
        progressTimer.restart()
        break;
      case Player.STATE_STOP:
        progressTimer.stop()
        buffer.text = ""
        progress.text = ""
        rate.text = ""
        error.text = ""
        break;
      case Player.STATE_PAUSED:
      case Player.STATE_PLAY:
      case Player.STATE_ENDED:
      default:
        break;
      }
    }

    onBuffering: {
      console.log('[mediaplayer] event buffering ' + percent)
      if (percent === 100) buffer.text = ""
      else buffer.text = "buffering " + percent + "%"
    }

    onError: {
      console.log('[mediaplayer] event error ' + type)
      switch (type) {
      case Player.ERROR_NETWORK:
        console.log("[mediaplayer] error network")
        if (error.text == "") error.text = "ERROR_NETWORK"
        break;
      case Player.ERROR_DECODE:
        console.log("[mediaplayer] error decode")
        if (error.text == "") error.text = "ERROR_DECODE"
        break;
      case Player.ERROR_SRC_NOT_SUPPORTED:
        console.log("[mediaplayer] error source not supported")
        if (error.text == "") error.text = "ERROR_SRC_NOT_SUPPORTED"
        break;
      default:
        break;
      }
    }
  }
}
