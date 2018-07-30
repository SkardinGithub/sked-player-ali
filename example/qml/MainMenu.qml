import QtQuick 2.0

Item {
    id: root
    width: 1280
    height: 720

    property int lastIndex: 0

    signal selected(string name)

    function showMenu() {
        loader.sourceComponent = menuComponent
        loader.focus = true
    }

    Loader {
        id: loader
    }

    Component {
        id: menuComponent

        Rectangle {
            width: 1280
            height: 720
            color: "transparent"

            Image {
                source: "../images/background.png"
                x:0
                y:0
                width:1280
                height:720
            }

            ListModel {
                id: menuModel
                ListElement {
                    name: "audio"
                    icon_blur: "../images/home/musics.png"
                    icon_focus: "../images/home/musics_selected.png"
                    title: "Audio"
                }
                ListElement {
                    name: "video"
                    icon_blur: "../images/home/movies.png"
                    icon_focus: "../images/home/movies_selected.png"
                    title: "Video"
                }
            }
            GridView {
                id: menuGridView
                width: 480
                height: 240
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                cellWidth: 240
                cellHeight: 240
                focus: true
                model: menuModel
                delegate: Column {
                    id: wrapper
                    spacing: 10

                    Item {
                        width: 120
                        height: 120
                        anchors.horizontalCenter: parent.horizontalCenter
                        Image {
                            anchors.fill: parent
                            anchors.centerIn: parent
                            source: wrapper.GridView.isCurrentItem ? icon_focus : icon_blur
                            smooth: true
                            fillMode: Image.PreserveAspectFit
                        }
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr(title)
                        color: wrapper.GridView.isCurrentItem ? "yellow" : "white"
                        smooth: true
                        font.pointSize: 24
                        //font.bold: true
                    }
                    transform: Scale {
                        id: scaleTransform
                        property real scale: 1.0
                        xScale: scale
                        yScale: scale
                        origin.x : wrapper.width/2
                        origin.y : wrapper.height/2
                    }
                    PropertyAnimation {
                        running: !wrapper.GridView.isCurrentItem
                        target: scaleTransform
                        properties: "scale"
                        to: 1.0
                        duration: 100
                    }
                    PropertyAnimation {
                        running: wrapper.GridView.isCurrentItem
                        target: scaleTransform
                        properties: "scale"
                        to: 1.15
                        duration: 300
                    }
                }
                currentIndex: root.lastIndex
                Keys.onPressed: {
                    if (event.key === Qt.Key_Return) {
                        event.accepted = true
                        root.lastIndex = currentIndex
                        var current = menuModel.get(currentIndex)
                        console.debug("[main menu] select " + current.name)
                        root.selected(current.name)
                        loader.sourceComponent = undefined
                    }
                }
            }
        }
    }
}
