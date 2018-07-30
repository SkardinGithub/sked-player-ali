import QtQuick 2.0

Item {
    x: 0
    y: 0
    width: 1280
    height: 160

    property string imageSource: ""
    property string title: ""

    Image {
        id: bg
        anchors.fill: parent;
        anchors.topMargin: 50
        anchors.bottomMargin: 50
        source: "../images/top_bar_bg.png";
        smooth: true

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 160
            text: qsTr(title)
            color: "white"
            font.pointSize: 28
            elide: Text.ElideLeft;
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter
        }
    }

    Item {
        width: 140
        height: 140
        anchors.left: bg.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter

        Image {
            anchors.centerIn: parent
            source: imageSource
            fillMode: Image.PreserveAspectFit
            smooth: true
        }
    }
}
