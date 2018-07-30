import QtQuick 2.7
import SKED.MediaPlayer 1.0

Rectangle {
    id: root
    color: "transparent"

    property var playlist
    property int currentPlayIndex: -1

    function start(index) {
        root.playlist.forEach(function(e) {
            playListModel.append(e)
        })
        playListView.currentIndex = index
        playListView.focus = true
        playIndex(index)
    }

    function playIndex(index) {
        currentPlayIndex = index
        playListView.currentIndex = index
        var current = playListModel.get(currentPlayIndex)
        Player.src = current.uri;
        Player.load();
        Player.play();
    }

    function playNext() {
        playIndex((currentPlayIndex + 1) % playListView.count)
    }

    function playPrev() {
        playIndex(currentPlayIndex > 0 ? currentPlayIndex -1 : playListView.count -1)
    }

    Image {
        source: "../images/background.png"
        x:0
        y:0
        width:1280
        height:720
    }

    Sound {
        id: snd
        width: parent.width
        height: 50
        anchors.top: parent.top
        anchors.topMargin: 80
        opacity: 0.7
        z: 2
    }

    TopBar {
        id: topBar
        imageSource: "../images/home/musics.png"
        title: "Audio"
    }

    Item {
        anchors.top: topBar.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.right: parent.right
        height: 400

        Rectangle {
            id: middleLine
            width: 2
            height: parent.height
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#353535"
            opacity: 0.7
        }

        ListView {
            id: playListView
            anchors.left: parent.left
            anchors.right: middleLine.left
            height: parent.height

            model: ListModel {
                id: playListModel
            }
            delegate: playlistDelegate
            highlight: Item {
                Image {
                    anchors.fill: parent
                    source: "../images/select_bar.png"
                    smooth: true
                }
                Image {
                    id: arrow
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 30
                    source: "../images/yellow_arrow.png"
                }
            }
            highlightMoveVelocity: 1000
            clip: true
            focus: true
            currentIndex: -1
            Keys.onPressed: {
                //console.debug('[audio playerlist] key pressed. code: ' + event.key)
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Right) {
                    event.accepted = true
                    playIndex(currentIndex)
                }
            }
        }

        Component {
            id: playlistDelegate
            Row {
                id: playlistDelegateItem
                width: playListView.width
                leftPadding: 40
                rightPadding: 40
                height: 40
                spacing: 10

                Item {
                    width: parent.height
                    height: parent.height
                    Image {
                        anchors.fill: parent
                        anchors.margins: 8
                        source: "../images/audio/current_playing_icon.png"
                        smooth: true
                        fillMode: Image.PreserveAspectFit
                        visible:  currentPlayIndex === index
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        if (playListModel.cnt < 10)
                            return (index + 1) + ". " + title
                        return ("0000" + (index + 1)).slice(-(parseInt(Math.log(playListModel.count)/Math.LN10)+1)) + ". " + title
                    }
                    color: playlistDelegateItem.ListView.isCurrentItem ? "yellow" : "white"
                    font.pointSize: 26
                    //font.bold: true
                    elide: Text.ElideRight
                }
            }
        }

        Item {
            anchors.right: parent.right
            anchors.left: middleLine.right
            anchors.leftMargin: 20
            anchors.rightMargin: 40
            height: parent.height

            Text {
                id: albumName
                width: parent.width
                height: 20
                font.pointSize: 26
                elide: Text.ElideMiddle
                text: ""
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "white"
            }

            Image {
                id: albumArt
                anchors.top: albumName.bottom
                anchors.topMargin: 5
                anchors.horizontalCenter: parent.horizontalCenter
                smooth: true
                width: 360
                height: 360
                source: "../images/audio/default_album_art.png"

                Image {
                    anchors.centerIn: parent
                    width: 120
                    height: 120
                    smooth: true
                    opacity: 0.7
                    source: Player.state === Player.STATE_PLAY ? "../images/audio/play_icon.png" : "../images/audio/pause_icon.png"
                    visible: Player.state === Player.STATE_PAUSED || (Player.state === Player.STATE_PLAY)
                }

                Rectangle {
                    height: 36
                    anchors.left: parent.left
                    anchors.leftMargin: 5
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.bottom: albumArt.bottom
                    anchors.bottomMargin: 5
                    color: "#0966CC"
                    opacity: 0.7
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 5
                        width: parent.width / 2
                        text: (currentPlayIndex >= 0) ? playListModel.get(currentPlayIndex).title : ""
                        font.pointSize: 18
                        elide: Text.ElideMiddle
                        color: "white"
                    }
                    Text {
                        id: progress
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 5
                        width: parent.width / 2
                        text: ""
                        font.pointSize: 18
                        color: "white"
                        horizontalAlignment: Text.AlignRight

                        Timer {
                            interval: 1000
                            triggeredOnStart: false
                            repeat: true
                            running: true
                            onTriggered: {
                                if (Player.state === Player.STATE_PLAY || Player.state === Player.STATE_PAUSED) {
                                    var duration = Player.duration
                                    var currentTime = Player.currentTime
                                    var str = ""
                                    if (currentTime >= 0)
                                        str += toHHMMSS(currentTime)
                                    if (duration >= 0)
                                        str += "/" + toHHMMSS(duration)
                                    progress.text = str
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
                        }
                    }
                }
            }
        }
    }

    Keys.onPressed: {
        console.debug('[audio player] key pressed. code: ' + event.key);
        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_Stop:
            console.debug('[audio player] stop');
            Player.stop();
            event.accepted = true;
            root.destroy()
            break;
        case Qt.Key_MediaPlay: // RCU PlAY_PAUSE
            console.debug('[audio player] play_pause');
            if (Player.state === Player.STATE_PLAY) Player.pause();
            else Player.play();
            break;
        case Qt.Key_VolumeUp:
            console.debug('[audio player] volume up');
            if (snd.getMute()) snd.setMute(false);
            snd.setVol(Math.min(snd.getVol() + 1, 100));
            break;
        case Qt.Key_VolumeDown:
            console.debug('[audio player] volume down');
            snd.setVol(Math.max(snd.getVol() - 1, 0));
            break;
        case Qt.Key_N: // RCU MUTE
            console.debug('[audio player] mute/unmute');
            snd.setMute(!snd.getMute());
            break;
        case Qt.Key_ChannelUp:
            console.debug('[audio player] Ch+')
            playNext();
            break;
        case Qt.Key_ChannelDown:
            console.debug('[audio player] Ch-')
            playPrev();
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    Connections {
        target: Player
        onStateChange: {
            console.debug('[audio player] event statechange ' + oldState + "->" + newState);
            switch (newState) {
            case Player.STATE_STOP:
                progress.text = ""
                break;
            case Player.STATE_ENDED:
                playNext();
                break;
            case Player.STATE_LOADED:
            case Player.STATE_PAUSED:
            case Player.STATE_PLAY:
            default:
                break;
            }
        }
    }

    Component.onDestruction: {
        console.debug("[audio player] onDestrution")
        Player.stop()
    }
}
