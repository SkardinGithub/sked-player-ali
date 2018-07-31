import QtQuick 2.7
import Qt.labs.folderlistmodel 2.1
import "Content.js" as Content

Rectangle {
    id: fileBrowser
    color: "transparent"
    x: 0
    y: 0
    width: 1280
    height: 720

    property string folder: "/media"
    property string rootFolder: ""
    property var currentIndex: [0]
    property string titleImageUri: ""
    property int video : 1
    property int audio : 2
    property int filter: audio | video

    signal open(var playlist, int startIndex)
    signal exit()

    function selectFile(file) {
        if (file !== "") {
            var folderListModel = loader.item.mainFolders
            var playlist = [];
            var startIndex = 0;
            for (var i = 0; i < folderListModel.count; i++) {
                if (!folderListModel.get(i, "fileIsDir")) {
                    var filePath = "file://" + folderListModel.get(i, "filePath")
                    var fileName = folderListModel.get(i, "fileName")
                    playlist.push({ "uri": filePath, "title": fileName })
                    if (filePath === file)
                        startIndex = playlist.length - 1;
                }
            }
            fileBrowser.open(playlist, startIndex)
        }
        fileBrowser.folder = loader.item.mainFolders.folder
        loader.sourceComponent = undefined
    }

    function getNameFilter() {
        var exts = [];
        if (filter & audio) exts.push.apply(exts, Content.audioExt);
        if (filter & video) exts.push.apply(exts, Content.videoExt);
        return exts.map(function(ext) { return "*." + ext });
    }

    Loader {
        id: loader
    }

    function show() {
        if (rootFolder === "")
            rootFolder = folder
        loader.sourceComponent = fileBrowserComponent
        loader.item.parent = fileBrowser
        loader.item.anchors.fill = fileBrowser
        loader.item.mainView.currentIndex = 0
        loader.item.mainView.focus = true
    }

    Component {
        id: fileBrowserComponent

        Item {
            id: root
            property variant mainFolders: folders1
            property variant subFolders: folders2
            property variant mainView: view1
            property variant subView: view2

            Image {
                source: "../images/background.png"
                anchors.fill: root
            }

            TopBar {
                id: titleBar
                anchors.top: parent.top
                imageSource: titleImageUri
                title:{
                    mainFolders.folder.toString().substring(rootFolder.length) || "/"
                }
            }

            Item {
                anchors.top: titleBar.bottom
                anchors.topMargin: 20
                height: 400
                anchors.left: parent.left
                anchors.right: parent.right

                Rectangle {
                    id: middleLine
                    width: 2
                    height: parent.height
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#353535"
                    opacity: 0.7
                }

                FolderListModel {
                    id: folders1
                    folder: fileBrowser.folder
                    showDirsFirst: true
                    sortField: FolderListModel.Name
                    nameFilters: getNameFilter()
                }

                FolderListModel {
                    id: folders2
                    folder: fileBrowser.folder
                    showDirsFirst: true
                    sortField: FolderListModel.Name
                    nameFilters: getNameFilter()
                }

                Component {
                    id: folderDelegate

                    Row {
                        id: wrapper
                        leftPadding: 40
                        rightPadding: 50
                        function launch() {
                            var path = "file://" + filePath;
                            if (fileIsDir)
                                root.down(path);
                            else {
                                fileBrowser.selectFile(path)
                            }
                        }
                        width: parent.width
                        height: 40
                        spacing: 10

                        Item {
                            width: parent.height
                            height: parent.height
                            Image {
                                source: {
                                    if (fileIsDir)
                                        return "../images/filebrowser/folder_icon.png"
                                    else if (Content.videoExt.indexOf(fileSuffix) != -1)
                                        return "../images/filebrowser/video_icon.png"
                                    else if (Content.audioExt.indexOf(fileSuffix) != -1)
                                        return "../images/filebrowser/audio_icon.png"
                                    else
                                        return ""
                                }
                                fillMode: Image.PreserveAspectFit
                                anchors.fill: parent
                                smooth: true
                                anchors.margins: 8
                            }
                        }

                        Text {
                            id: nameText
                            verticalAlignment: Text.AlignVCenter
                            text: fileName
                            font.pointSize: 26
                            color: wrapper.ListView.view === mainView && wrapper.ListView.isCurrentItem ? "yellow" : "white"
                            elide: Text.ElideRight
                        }
                    }
                }

                ListView {
                    id: view1
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    x: 0
                    width: parent.width / 2
                    clip: true
                    model: folders1
                    currentIndex: -1
                    delegate: folderDelegate
                    highlight: Item {
                        width: view1.currentItem == null ? 0 : view1.currentItem.width
                        visible: view1.count != 0 && view1.state === "current";
                        Image {
                            anchors.fill: parent
                            source: "../images/select_bar.png"
                            smooth: true
                        }
                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 30
                            source: "../images/yellow_arrow.png"
                        }
                    }
                    highlightMoveVelocity: 1000
                    focus: true
                    state: "current"
                    states: [
                        State {
                            name: "current"
                            PropertyChanges { target: view1; x: 0 }
                        },
                        State {
                            name: "sub"
                            PropertyChanges { target: view1; x: root.width / 2 }
                        }
                    ]
                    transitions: [
                        Transition {
                            NumberAnimation { properties: "x"; duration: 250;  easing.type: Easing.Linear }
                        }
                    ]
                    onCurrentIndexChanged: {
                        if (view1 === mainView) root.onCurrentIndexChanged()
                    }
                }

                ListView {
                    id: view2
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    x: parent.width / 2
                    width: parent.width / 2
                    visible: false
                    clip: true
                    model: folders2
                    delegate: folderDelegate
                    highlight: Item {
                        width: view2.currentItem == null ? 0 : view2.currentItem.width
                        visible: view2.count != 0 && view2.state === "current";
                        Image {
                            anchors.fill: parent
                            source: "../images/select_bar.png"
                            smooth: true
                        }
                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 30
                            source: "../images/yellow_arrow.png"
                        }
                    }
                    highlightMoveVelocity: 1000
                    state: "sub"
                    states: [
                        State {
                            name: "current"
                            PropertyChanges { target: view2; x: 0 }
                        },
                        State {
                            name: "sub"
                            PropertyChanges { target: view2; x: root.width / 2}
                        }
                    ]
                    transitions: [
                        Transition {
                            NumberAnimation { properties: "x"; duration: 250;  easing.type: Easing.Linear }
                        }
                    ]
                    onCurrentIndexChanged: {
                        if (view2 === mainView) root.onCurrentIndexChanged()
                    }
                }

                ListView {
                    id: fileInfoView
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    x: parent.width / 2
                    width: parent.width / 2
                    visible: false
                    clip: true
                    keyNavigationEnabled: false
                    model: ListModel {
                        id: fileInfoModel
                    }
                    delegate: Item {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 40
                        anchors.rightMargin: 40
                        height: 40
                        Text {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            text: {
                                var str = name + ": ";
                                if (name.toString() === "size") {
                                    var size = Number(value)
                                    if (size >= 1024 * 1024)
                                        str += (size / (1024 * 1024)).toFixed(1) + "M"
                                    else if (size >= 1024)
                                        str += (size / 1024).toFixed(1) + "K"
                                } else {
                                    str += value
                                }
                                return str
                            }
                            font.pointSize: 24
                            color: "white"
                            elide: Text.ElideRight
                        }
                    }
                }

                Keys.onPressed: {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Select || event.key === Qt.Key_Right) {
                        mainView.currentItem.launch();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Left) {
                        root.up();
                    }
                }
            }

            function down(path) {
                if (subView.count <= 0)
                    return
                if (mainFolders == folders1) {
                    mainView = view2
                    subView = view1
                    mainFolders = folders2;
                    subFolders = folders1;
                } else {
                    mainView = view1
                    subView = view2
                    mainFolders = folders1;
                    subFolders = folders2;
                }
                mainView.x = root.width / 2;
                mainView.state = "current";
                mainView.focus = true;
                fileBrowser.currentIndex.push(0)
                mainFolders.folder = path;
                mainView.currentIndex = 0
                mainView.visible = true
                subView.x = root.width
                subView.state = "sub";
                subView.visible = false
                fileInfoView.visible = false
                updateSubView()
            }

            function up() {
                if (fileBrowser.currentIndex.length <= 1) {
                    //fileBrowser.exit()
                    return
                }
                var path = mainFolders.parentFolder;
                if (path.toString().length === 0 || path.toString() === 'file:')
                    return;
                if (mainFolders == folders1) {
                    mainView = view2
                    subView = view1
                    mainFolders = folders2;
                    subFolders = folders1;
                } else {
                    mainView = view1
                    subView = view2
                    mainFolders = folders1;
                    subFolders = folders2;
                }
                mainView.x = -root.width / 2;
                mainView.state = "current";
                mainView.focus = true;
                fileBrowser.currentIndex.pop()
                var index = fileBrowser.currentIndex[fileBrowser.currentIndex.length -1]
                mainFolders.folder = path;
                mainView.currentIndex = index
                mainView.visible = true
                subView.x = 0
                subView.state = "sub";
                subView.visible = true
                fileInfoView.visible = false
                updateSubView()
            }

            function onCurrentIndexChanged() {
                mainView.visible = true
                fileBrowser.currentIndex[fileBrowser.currentIndex.length - 1] = mainView.currentIndex
                updateSubView()
            }

            function updateSubView() {
                var path = "file://" + mainView.model.get(mainView.currentIndex, "filePath")
                if (mainFolders.folder.toString() !== path.substring(0, path.lastIndexOf('/'))) {
                    // FolderListModel not updated yet
                    subViewTimer.restart()
                    return
                }
                if (mainView.model.get(mainView.currentIndex, "fileIsDir")) {
                    fileInfoView.visible = false
                    subFolders.folder = path
                    subView.currentIndex = 0
                    subView.visible = true
                } else {
                    subView.visible = false
                    fileInfoModel.clear()
                    fileInfoModel.append({"name": "name", "value": mainView.model.get(mainView.currentIndex, "fileName")})
                    fileInfoModel.append({"name": "size", "value": mainView.model.get(mainView.currentIndex, "fileSize").toString()})
                    fileInfoView.visible = true
                }
            }

            Timer {
                id: subViewTimer
                interval: 250
                triggeredOnStart: false
                repeat: false
                running: false
                onTriggered: updateSubView()
            }
        }
    }
}
