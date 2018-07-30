import QtQuick 2.0
import SKED.MediaPlayer 1.0

Item {
    id: snd
    visible: false
    //opacity: 0.7

    function getVol() {
        return Math.round(Player.vol * 100);
    }

    function setVol(vol) {
        Player.vol = vol / 100.0;
        snd.show();
    }

    function getMute() {
        return Player.mute;
    }

    function setMute(on) {
        Player.mute = on;
        snd.show();
    }

    function show() {
        snd.visible = true;
        snd_timer.restart();
    }

    Rectangle {
        anchors.fill: parent
        color: "#808080"
        Text {
            id: snd_txt
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 60
            width: 40
            text: Math.round(Player.vol * 100)
            font.pointSize: 26
            //font.bold: true
            color: Player.mute ? "#A0A0A0" : "#FFFFFF"
        }
        Image {
            id: snd_icon
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: snd_txt.left
            anchors.rightMargin: 20
            width: 32
            source: Player.mute ? "../images/snd/mute_icon.png" : "../images/snd/vol_icon.png"
        }
        Rectangle {
            id: snd_bar
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 60
            anchors.right: snd_icon.left
            anchors.rightMargin: 20
            height: 25
            color: "#303030"
            Rectangle {
                anchors.left: parent.left
                color: Player.mute ? "#A0A0A0" : "#FFFFFF"
                width: Math.round(Player.vol * 100) * parent.width / 100
                height: parent.height
            }
        }
    }

    Timer {
        id: snd_timer
        interval: 2000
        triggeredOnStart: false
        repeat: false
        running: false
        onTriggered: {
            snd.visible = false
        }
    }
}
