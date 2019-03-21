import QtQuick 2.0
import SKED.MediaPlayer 1.0

Rectangle {
    id: root
    color: "transparent"
    focus: true

    property double itemOpacity: 0.7
    property var playlist: []
    property int currentPlayIndex: -1

    property string src: ""
    property string title: ""

    function start(index) {
        play(index)
    }

    function play(index) {
        currentPlayIndex = index
        Player.src = playlist[currentPlayIndex]["uri"];
        console.log("last stop time: " + Player.getLastStopTime(Player.src));
        Player.load();
        Player.play();
        control.show();
        root.focus = true;
    }

    function playNext() {
        if (currentPlayIndex < playlist.length -1)
            play(currentPlayIndex+1)
    }

    function playPrev() {
        if (currentPlayIndex > 0)
            play(currentPlayIndex-1)
    }

    Sound {
        id: snd
        width: parent.width
        height: 50
        anchors.top: parent.top
        anchors.topMargin: 80
        opacity: 0.7
    }

    Item {
        id: control
        width: parent.width
        y: 720
        height: 220
        opacity: itemOpacity

        property int toPosition: -1

        function show() {
            control.state = "show"
            control_timer.restart();
        }

        function hide() {
            if (Player.state === Player.STATE_PLAY) {
                control.state = "hide"
            }
        }

        function stop() {
            Player.stop();
        }

        function play() {
            Player.play();
        }

        function pause() {
            Player.pause();
        }

        function faster() {
            switch (Player.rate) {
            case -2: Player.rate = 1; break;
            case 1: Player.rate = 2; break;
            default: Player.rate += 2;
            }
            show()
        }

        function slower() {
            switch (Player.rate) {
            case 1: Player.rate = -2; break;
            case 2: Player.rate = 1; break;
            default: Player.rate -= 2;
            }
            show()
        }

        function seekForWard() {
            var duration = Player.duration
            var currentTime = Player.currentTime
            if (duration > 0) {
                var step = Math.min(30, duration / 10)
                if (control.toPosition < 0) {
                    if (currentTime > 0) control.toPosition = currentTime
                    else control.toPosition = 0
                }
                if (control.toPosition < duration - step - 5)
                    control.toPosition += step;
                progress_timer.stop()
                progress_bar.render(control.toPosition , duration)
                seek_timer.restart()
                show()
            }
        }

        function seekBackWard() {
            var duration = Player.duration
            var currentTime = Player.currentTime
            if (duration > 0) {
                var step = Math.min(30, duration / 10)
                if (control.toPosition < 0) {
                    if (currentTime > 0) control.toPosition = currentTime
                    else control.toPosition = 0
                }
                if (control.toPosition >= step)
                    control.toPosition -= step;
                else
                    control.toPosition = 0;
                progress_timer.stop()
                progress_bar.render(control.toPosition , duration)
                seek_timer.restart()
                show()
            }
        }

        function toHHMMSS(sec_num) {
            var hours   = Math.floor(sec_num / 3600);
            var minutes = Math.floor((sec_num - (hours * 3600)) / 60);
            var seconds = Math.floor(sec_num - (hours * 3600) - (minutes * 60));
            if (hours   < 10) {hours   = "0"+hours;}
            if (minutes < 10) {minutes = "0"+minutes;}
            if (seconds < 10) {seconds = "0"+seconds;}
            return hours+':'+minutes+':'+seconds;
        }

        Rectangle {
            anchors.fill: parent
            color: "#808080"
        }

        Item {
            anchors.centerIn: parent
            width: parent.width - 120
            Image {
                id: state_icon
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                source: Player.state === Player.STATE_PLAY ? "../images/video/play_icon.png" : "../images/video/pause_icon.png"
                visible: Player.state === Player.STATE_PAUSED || (Player.state === Player.STATE_PLAY && Player.bufferLevel === 100)
                smooth: true
            }

            Image {
                id: loading_icon
                source: "../images/video/loading_icon.png"
                anchors.centerIn: state_icon
                visible: (Player.state === Player.STATE_LOADED || Player.state === Player.STATE_PLAY) && (Player.bufferLevel !== 100)
                NumberAnimation on rotation {
                    running: visible; from: 0; to: 360;
                    loops: Animation.Infinite; duration: 1500
                }
            }

            Item {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - state_icon.width - 40
                height: 90

                Text {
                    id: title_txt
                    anchors.top: parent.top
                    anchors.left: parent.left
                    height: 40
                    width: parent.width - 100
                    color: "yellow"
                    text: {
                      if (currentPlayIndex >= 0) return playlist[currentPlayIndex]["title"];
                      else return ""
                    }
                    font.pointSize: 26
                    //font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideMiddle
                }

                Text {
                    id: speed_txt
                    anchors.top: parent.top
                    anchors.right: parent.right
                    height: 40
                    color: "yellow"
                    text: Player.rate + "X"
                    font.pointSize: 26
                    //font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    visible: Player.rate !== 1.0
                }

                Rectangle {
                    id: progress_bar
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 40
                    color: "#303030"
                    function render(position, duration) {
                        if (Player.state === Player.STATE_PLAY) {
                            if (duration >= 0) duration_txt.text = control.toHHMMSS(duration)
                            if (position >= 0) current_time_txt.text = control.toHHMMSS(position)
                            if (duration > 0 && position >=0 && position <= duration) {
                                progress_bar_position.width = position / duration * progress_bar.width
                            } else {
                                progress_bar_position.width = 0;
                            }
                        }
                    }

                    Rectangle {
                        id: progress_bar_position
                        anchors.left: parent.left
                        color: "#FFFFFF"
                        width: 0
                        height: parent.height
                    }
                    Text {
                        id: current_time_txt
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        color: "yellow"
                        font.pointSize: 26
                        //font.bold: true
                        text: "loading..."
                    }
                    Text {
                        id: duration_txt
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        color: "yellow"
                        font.pointSize: 26
                        //font.bold: true
                        text: ""
                    }
                    Timer {
                        id: progress_timer
                        interval: 1000
                        triggeredOnStart: false
                        repeat: true
                        running: true
                        onTriggered: progress_bar.render(Player.currentTime, Player.duration)
                    }
                    Timer {
                        id: seek_timer
                        interval: 1000
                        triggeredOnStart: false
                        repeat: false
                        running: false
                        onTriggered: {
                            progress_timer.restart()
                            console.debug('[video player] seek to ' + control.toPosition)
                            Player.currentTime = control.toPosition
                        }
                    }
                }
            }
        }

        Timer {
            id: control_timer
            interval: 5000
            triggeredOnStart: false
            repeat: false
            running: false
            onTriggered: {
                if (Player.state === Player.STATE_PLAY) {
                    control.state = "hide"
                } else {
                    restart()
                }
            }
        }
        states: [
            State {
                name: "show"
                PropertyChanges { target: control; y: 500 }
            },
            State {
                name: "hide"
                PropertyChanges { target: control; y: 720}
            }
        ]
        transitions: [
            Transition {
                to: "hide"
                NumberAnimation { properties: "y"; duration: 300;  easing.type: Easing.Linear }
            }
        ]
    }

    Keys.onPressed: {
        console.debug('[video player] key pressed. code: ' + event.key);
        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_Stop:
            console.debug('[video player] stop');
            control.stop();
            event.accepted = true;
            root.destroy()
            break;
        case Qt.Key_Return:
        case Qt.Key_MediaPlay: // RCU PlAY_PAUSE
            console.debug('[video player] play_pause');
            if (Player.state === Player.STATE_PLAY) control.pause();
            else control.play();
            break;
        case Qt.Key_VolumeUp:
            console.debug('[video player] volume up');
            if (snd.getMute()) snd.setMute(false);
            snd.setVol(Math.min(snd.getVol() + 1, 100));
            break;
        case Qt.Key_VolumeDown:
            console.debug('[video player] volume down');
            snd.setVol(Math.max(snd.getVol() - 1, 0));
            break;
        case Qt.Key_N: // RCU MUTE
            console.debug('[video player] mute/unmute');
            snd.setMute(!snd.getMute());
            break;
        case Qt.Key_Up:
            control.show();
            break;
        case Qt.Key_Down:
            control.hide();
            break;
        case Qt.Key_Left:
            control.seekBackWard();
            break;
        case Qt.Key_Right:
            control.seekForWard();
            break;
        case Qt.Key_MediaPrevious: // RCU FRW
            control.slower();
            break;
        case Qt.Key_MediaNext: // RCU FFW
            control.faster();
            break;
        case Qt.Key_ChannelUp:
            playNext();
            break;
        case Qt.Key_ChannelDown:
            playPrev();
            break;
        case Qt.Key_0:
            Player.fullscreen = true;
            break;
        case Qt.Key_1:
            Player.displayrect = Qt.rect(0, 0, 640, 360);
            if (Player.fullscreen) Player.fullscreen = false;
            break;
        case Qt.Key_2:
            Player.displayrect = Qt.rect(640, 0, 640, 360);
            if (Player.fullscreen) Player.fullscreen = false;
            break;
        case Qt.Key_3:
            Player.displayrect = Qt.rect(0, 360, 640, 360);
            if (Player.fullscreen) Player.fullscreen = false;
            break;
        case Qt.Key_4:
            Player.displayrect = Qt.rect(640, 360, 640, 360);
            if (Player.fullscreen) Player.fullscreen = false;
            break;
        case Qt.Key_5:
            Player.displayrect = Qt.rect(320, 180, 640, 360);
            if (Player.fullscreen) Player.fullscreen = false;
            break;
        case Qt.Key_6:
            Player.displayrect = Qt.rect(650, 220, 320, 204);
            if (Player.fullscreen) Player.fullscreen = false;
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    Connections {
        target: Player
        onStateChange: {
            console.debug('[video player] event statechange ' + oldState + "->" + newState);
            switch (newState) {
            case Player.STATE_STOP:
                //control.show();
                break;
            case Player.STATE_ENDED:
                if (currentPlayIndex >= playlist.length - 1)
                    root.destroy()
                else
                    playNext()
                break;
            case Player.STATE_LOADED:
            case Player.STATE_PAUSED:
                control.show();
                break;
            case Player.STATE_PLAY:
            default:
                break;
            }
        }
    }

    Component.onCompleted: {
        control.show()
    }

    Component.onDestruction: {
        console.debug("[video player] onDestrution")
        Player.stop()
    }
}
