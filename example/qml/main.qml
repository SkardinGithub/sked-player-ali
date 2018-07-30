import QtQuick 2.0

Item {
    id: root
    width: 1280
    height: 720
    property var launchedObj: null

    function openMenu(name) {
        if (name === "audio" || name === "video") {
            var component = Qt.createComponent("VfsBrowser.qml");
            if (component.status === Component.Error) {
                console.debug("[main] Error: "+ component.errorString());
                return;
            }
            launchedObj = component.createObject(root);
            if (name === "video") launchedObj.filter = launchedObj.video
            else if (name === "audio") launchedObj.filter = launchedObj.audio
            launchedObj.Component.onDestruction.connect(onAppExit)
        }
    }

    function onAppExit() {
        launchedObj = null
        mainMenu.showMenu()
    }

    MainMenu {
        id: mainMenu
        Component.onCompleted: selected.connect(root.openMenu)
    }

    Component.onCompleted: {
        mainMenu.showMenu()
    }

    Keys.onPressed: {
        console.debug("[main] key: " + event.key)
        if (event.key === 77) {
            event.accepted = true
            if (launchedObj) {
                launchedObj.destroy()
                launchedObj = null
            }
        }
    }
}
