import QtQuick 2.0

Item {
    id: root
    width: 1280
    height: 720

    property int video: 1
    property int audio: 2
    property int filter: root.video
    property var contentObj: null;

    FileBrowser {
        id: fileBrowser
        folder: "file:///mnt/usb"
        Component.onCompleted: open.connect(root.openContent)
        titleImageUri: {
            if (filter === root.video) return "../images/home/movies.png";
            else if (filter === root.audio) return "../images/home/musics.png";
            return "";
        }
        filter: {
            if (root.filter === root.video) return fileBrowser.video
            else if (root.filter === root.audio)  return fileBrowser.audio
            return 0;
        }
        onExit: {
            root.destroy()
        }
    }

    function openContent(playlist, startIndex) {
        if (root.filter === root.video) openVideo(playlist, startIndex);
        else if (root.filter === root.audio) openAudio(playlist, startIndex)
    }

    function openVideo(playlist, startIndex) {
        console.debug('[Vfs Browser] open video')
        var component = Qt.createComponent("ContentVideo.qml");
        if (component.status === Component.Error) {
            console.debug("[Vfs Browser] Error: "+ component.errorString());
            return;
        }
        contentObj = component.createObject(root);
        contentObj.anchors.fill = root
        contentObj.Component.onDestruction.connect(root.onContentExit)
        contentObj.playlist = new Object(playlist)
        contentObj.start(startIndex)
    }

    function openAudio(playlist, startIndex) {
        console.debug('[Vfs Browser] open audio')
        var component = Qt.createComponent("ContentAudio.qml");
        if (component.status === Component.Error) {
            console.debug("[Vfs Browser] Error: "+ component.errorString());
            return;
        }
        contentObj = component.createObject(root);
        contentObj.anchors.fill = root
        contentObj.Component.onDestruction.connect(root.onContentExit)
        contentObj.playlist = new Object(playlist)
        contentObj.start(startIndex)
    }

    function onContentExit() {
        console.debug("[Vfs Browser] content exit")
        contentObj = null
        fileBrowser.show()
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Escape) {
            event.accepted = true
            root.destroy()
        }
    }

    Component.onCompleted: {
        fileBrowser.show()
    }

    Component.onDestruction: {
        console.debug("[Vfs Browser] onDestruction")
        if (contentObj) {
            contentObj.Component.onDestruction.disconnect(root.onContentExit)
        }
    }
}
