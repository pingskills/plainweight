import PlainWeight.Core
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    property bool editing: false
    property string originalDate: ""
    property string pendingDate: ""
    property string selectedDate: ""
    property int period: 1
    property bool showTrend: true
    property string statusText: ""

    function filePath(url) {
        return decodeURIComponent(url.toString().replace(/^file:\/\//, ""));
    }

    function resetEntry() {
        editing = false;
        dateField.text = App.today();
        weightField.text = "";
        weightField.forceActiveFocus();
    }

    function editEntry(date) {
        editing = true;
        originalDate = date;
        dateField.text = date;
        weightField.text = App.weightOn(date);
        weightField.forceActiveFocus();
        weightField.selectAll();
    }

    function saveEntry() {
        if (App.exists(dateField.text) && !editing) {
            pendingDate = dateField.text;
            updateDialog.open();
            return ;
        }
        let saved = editing ? App.edit(originalDate, dateField.text, weightField.text) : App.save(dateField.text, weightField.text, false);
        if (saved) {
            statusText = "Weight saved";
            resetEntry();
        } else {
            statusText = App.error;
        }
    }

    function selectedEdit() {
        if (history.currentIndex >= 0)
            editEntry(App.historyDate(history.currentIndex));

    }

    function selectedDelete() {
        if (history.currentIndex >= 0) {
            selectedDate = App.historyDate(history.currentIndex);
            deleteDialog.open();
        }
    }

    objectName: "mainWindow"
    width: 900
    height: 820
    minimumWidth: 620
    minimumHeight: 560
    visible: true
    title: "PlainWeight"
    color: Theme.background
    palette.window: Theme.background
    palette.windowText: Theme.text
    palette.base: Theme.input
    palette.text: Theme.text
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    Component.onCompleted: resetEntry()

    Shortcut {
        sequence: "Ctrl+N"
        onActivated: window.resetEntry()
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: window.saveEntry()
    }

    Shortcut {
        sequence: "Ctrl+E"
        onActivated: window.selectedEdit()
    }

    Shortcut {
        sequence: "Ctrl+B"
        onActivated: backupFile.open()
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "Delete"
        onActivated: {
            if (history.activeFocus) {
                window.selectedDelete();
            }
        }
    }

    FileDialog {
        id: backupFile

        title: qsTr("Backup Data")
        fileMode: FileDialog.SaveFile
        nameFilters: ["SQLite database (*.db)"]
        currentFile: "plainweight-backup-" + App.today() + ".db"
        onAccepted: window.statusText = App.backup(window.filePath(selectedFile)) ? qsTr("Backup saved") : App.error
    }

    FileDialog {
        id: restoreFile

        title: qsTr("Restore Data")
        fileMode: FileDialog.OpenFile
        nameFilters: ["SQLite database (*.db)"]
        onAccepted: {
            let path = window.filePath(selectedFile);
            let summary = App.inspectBackup(path);
            if (summary) {
                window.pendingDate = path;
                restoreDialog.text = qsTr("Restore %1? A safety copy of your current data will be saved first.").arg(summary);
                restoreDialog.open();
            } else {
                window.statusText = App.error;
            }
        }
    }

    FileDialog {
        id: exportFile

        title: qsTr("Export CSV")
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV (*.csv)"]
        currentFile: "plainweight-" + App.today() + ".csv"
        onAccepted: window.statusText = App.exportCsv(window.filePath(selectedFile)) ? qsTr("CSV exported") : App.error
    }

    FileDialog {
        id: importFile

        title: qsTr("Import CSV")
        fileMode: FileDialog.OpenFile
        nameFilters: ["CSV (*.csv)"]
        onAccepted: {
            window.pendingDate = window.filePath(selectedFile);
            importDialog.open();
        }
    }

    MessageDialog {
        id: updateDialog

        title: qsTr("Update existing entry?")
        text: qsTr("There is already a weight for this date. Replace it?")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button) {
            if (button === MessageDialog.Yes) {
                window.statusText = App.save(window.pendingDate, weightField.text, true) ? qsTr("Weight updated") : App.error;
                if (window.statusText === qsTr("Weight updated"))
                    window.resetEntry();

            }
        }
    }

    MessageDialog {
        id: deleteDialog

        title: qsTr("Delete entry?")
        text: qsTr("Delete the measurement on %1?").arg(window.selectedDate)
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button) {
            if (button === MessageDialog.Yes)
                window.statusText = App.remove(window.selectedDate) ? qsTr("Entry deleted") : App.error;

        }
    }

    MessageDialog {
        id: restoreDialog

        title: qsTr("Restore Data?")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button) {
            if (button === MessageDialog.Yes)
                window.statusText = App.restore(window.pendingDate) ? qsTr("Data restored; safety copy saved in the data folder") : App.error;

        }
    }

    MessageDialog {
        id: importDialog

        title: qsTr("Import CSV?")
        text: qsTr("Matching dates with identical weights will be skipped. A date with a different weight will stop the import without changing any data.")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function(button) {
            if (button === MessageDialog.Yes)
                window.statusText = App.importCsv(window.pendingDate) ? qsTr("CSV imported") : App.error;

        }
    }

    Dialog {
        id: targetDialog

        title: qsTr("Target weight")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        onAccepted: window.statusText = App.setTarget(targetField.text) ? qsTr("Target updated") : App.error

        contentItem: ColumnLayout {
            spacing: 8

            Label {
                text: qsTr("Target in kg. Leave empty to remove.")
            }

            TextField {
                id: targetField

                Layout.preferredWidth: 240
                placeholderText: "75.0"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }

        }

    }

    Dialog {
        id: aboutDialog

        title: qsTr("About PlainWeight")
        modal: true
        standardButtons: Dialog.Close
        anchors.centerIn: parent
        width: 430

        contentItem: Label {
            text: "PlainWeight 0.1.0\nLocal weight tracking for Linux\n\nData: " + App.dataPath()
            wrapMode: Text.Wrap
        }

    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 22

            Item {
                height: 3
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                spacing: 5

                Label {
                    text: qsTr("PlainWeight")
                    color: Theme.text
                    font.pointSize: Theme.fontSize * 1.8
                    font.weight: Font.DemiBold
                }

                Label {
                    text: qsTr("A clear view of your weight over time")
                    color: Theme.muted
                }

            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                height: 1
                color: Theme.border
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                spacing: 9

                Label {
                    text: window.editing ? qsTr("Edit weight") : qsTr("Record weight")
                    font.pointSize: Theme.fontSize * 1.2
                    font.weight: Font.DemiBold
                    color: Theme.text
                }

                RowLayout {
                    spacing: 10

                    TextField {
                        id: dateField

                        objectName: "dateField"
                        Layout.preferredWidth: 145
                        placeholderText: "YYYY-MM-DD"
                        font.features: {
                            "tnum": 1
                        }
                        Accessible.name: qsTr("Measurement date")
                        onAccepted: weightField.forceActiveFocus()
                    }

                    TextField {
                        id: weightField

                        objectName: "weightField"
                        Layout.preferredWidth: 170
                        placeholderText: "82.4"
                        font.pointSize: Theme.fontSize * 1.5
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        font.features: {
                            "tnum": 1
                        }
                        Accessible.name: qsTr("Weight in kilograms")
                        onAccepted: window.saveEntry()
                    }

                    Label {
                        text: "kg"
                        color: Theme.muted
                    }

                    Button {
                        text: window.editing ? qsTr("Save changes") : qsTr("Save weight")
                        highlighted: true
                        onClicked: window.saveEntry()
                        Accessible.name: text
                    }

                    Button {
                        text: qsTr("Cancel")
                        visible: window.editing
                        onClicked: window.resetEntry()
                    }

                }

                Label {
                    text: qsTr("Date defaults to today. Enter a past date to add history.")
                    color: Theme.muted
                    font.pointSize: Theme.fontSize * 0.9
                }

                Label {
                    text: window.statusText || App.error
                    visible: text.length > 0
                    color: Theme.text
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                height: 1
                color: Theme.border
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                spacing: 10

                Label {
                    text: qsTr("CURRENT WEIGHT")
                    color: Theme.muted
                    font.letterSpacing: 1.5
                    font.pointSize: Theme.fontSize * 0.85
                }

                RowLayout {
                    spacing: 16

                    Label {
                        text: App.latest
                        color: Theme.accent
                        font.pointSize: Theme.fontSize * 2.5
                        font.weight: Font.Medium
                        font.features: {
                            "tnum": 1
                        }
                    }

                    Label {
                        text: App.latestDate
                        color: Theme.muted
                        Layout.alignment: Qt.AlignBottom
                    }

                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 26

                    ColumnLayout {
                        Label {
                            text: qsTr("Since previous")
                            color: Theme.muted
                        }

                        Label {
                            text: App.previousChange
                            color: Theme.text
                        }

                    }

                    ColumnLayout {
                        Label {
                            text: qsTr("About 7 days")
                            color: Theme.muted
                        }

                        Label {
                            text: App.weekChange
                            color: Theme.text
                        }

                    }

                    ColumnLayout {
                        Label {
                            text: qsTr("About 30 days")
                            color: Theme.muted
                        }

                        Label {
                            text: App.monthChange
                            color: Theme.text
                        }

                    }

                }

            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                height: 1
                color: Theme.border
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Weight over time")
                        color: Theme.text
                        font.pointSize: Theme.fontSize * 1.2
                        font.weight: Font.DemiBold
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    CheckBox {
                        text: qsTr("Trend")
                        checked: window.showTrend
                        onToggled: window.showTrend = checked
                    }

                    ComboBox {
                        model: ["1 month", "3 months", "6 months", "1 year", "All time"]
                        currentIndex: window.period
                        onActivated: window.period = currentIndex
                        Accessible.name: qsTr("Graph period")
                    }

                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220

                    Canvas {
                        id: chart

                        property int shownCount: 0
                        property var values: App.graph
                        property int range: window.period
                        property bool trend: window.showTrend
                        property string target: App.target
                        property color lineColor: Theme.accent
                        property color mutedColor: Theme.muted
                        property color gridColor: Theme.border

                        anchors.fill: parent
                        onValuesChanged: requestPaint()
                        onRangeChanged: requestPaint()
                        onTrendChanged: requestPaint()
                        onTargetChanged: requestPaint()
                        onLineColorChanged: requestPaint()
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            let c = getContext("2d");
                            c.clearRect(0, 0, width, height);
                            let all = values || [];
                            if (!all.length) {
                                shownCount = 0;
                                return ;
                            }
                            let latest = new Date();
                            latest.setHours(12, 0, 0, 0);
                            let start = new Date(latest);
                            if (range === 0)
                                start.setMonth(start.getMonth() - 1);
                            else if (range === 1)
                                start.setMonth(start.getMonth() - 3);
                            else if (range === 2)
                                start.setMonth(start.getMonth() - 6);
                            else if (range === 3)
                                start.setFullYear(start.getFullYear() - 1);
                            else
                                start = new Date(all[0].date + "T12:00:00");
                            let shown = all.filter((p) => {
                                return new Date(p.date + "T12:00:00") >= start;
                            });
                            shownCount = shown.length;
                            if (!shown.length)
                                return ;

                            let min = shown[0].kg;
                            let max = shown[0].kg;
                            for (const point of shown) {
                                min = Math.min(min, point.kg);
                                max = Math.max(max, point.kg);
                            }
                            if (target !== "") {
                                min = Math.min(min, Number(target));
                                max = Math.max(max, Number(target));
                            }
                            let pad = Math.max(0.5, (max - min) * 0.15);
                            min -= pad;
                            max += pad;
                            let left = 48, right = 12, top = 14, bottom = 30, pw = width - left - right, ph = height - top - bottom;
                            let from = start.getTime(), to = latest.getTime();
                            if (to <= from)
                                to = from + 8.64e+07;
                            const mapX = (p) => left + (new Date(p.date + "T12:00:00").getTime() - from) / (to - from) * pw;
                            const mapY = (v) => top + (max - v) / (max - min) * ph;

                            c.font = "11px sans-serif";
                            c.fillStyle = mutedColor;
                            for (let i = 0; i < 3; i++) {
                                let val = min + (max - min) * i / 2, yy = mapY(val);
                                c.strokeStyle = gridColor;
                                c.beginPath();
                                c.moveTo(left, yy);
                                c.lineTo(width - right, yy);
                                c.stroke();
                                c.fillText(val.toFixed(1), 2, yy + 4);
                            }
                            let startLabel = Qt.formatDate(start, "dd MMM yyyy");
                            let endLabel = Qt.formatDate(latest, "dd MMM yyyy");
                            c.fillText(startLabel, left, height - 6);
                            c.fillText(endLabel, width - right - c.measureText(endLabel).width, height - 6);
                            if (target !== "") {
                                c.strokeStyle = mutedColor;
                                c.setLineDash([5, 4]);
                                c.beginPath();
                                c.moveTo(left, mapY(Number(target)));
                                c.lineTo(width - right, mapY(Number(target)));
                                c.stroke();
                                c.setLineDash([]);
                            }
                            if (trend) {
                                let prev = null;
                                c.strokeStyle = mutedColor;
                                c.lineWidth = 2;
                                for (const p of shown) {
                                    if (p.trend === undefined) {
                                        prev = null;
                                        continue;
                                    }
                                    if (prev) {
                                        c.beginPath();
                                        c.moveTo(mapX(prev), mapY(prev.trend));
                                        c.lineTo(mapX(p), mapY(p.trend));
                                        c.stroke();
                                    }
                                    prev = p;
                                }
                            }
                            c.strokeStyle = lineColor;
                            c.lineWidth = 2;
                            c.beginPath();
                            shown.forEach((p, i) => {
                                if (i)
                                    c.lineTo(mapX(p), mapY(p.kg));
                                else
                                    c.moveTo(mapX(p), mapY(p.kg));
                            });
                            c.stroke();
                            c.fillStyle = lineColor;
                            for (const p of shown) {
                                c.beginPath();
                                c.arc(mapX(p), mapY(p.kg), 3.5, 0, Math.PI * 2);
                                c.fill();
                            }
                        }

                    }

                    Label {
                        anchors.centerIn: parent
                        visible: chart.shownCount === 0
                        text: App.graph.length === 0 ? qsTr("Your measurements will appear here") : qsTr("No measurements in this period")
                        color: Theme.muted
                    }

                }

                Label {
                    text: qsTr("Dots are recorded measurements. The optional line is a 7-day average; dashed line is your target.")
                    color: Theme.muted
                    font.pointSize: Theme.fontSize * 0.9
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                height: 1
                color: Theme.border
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                spacing: 8

                Label {
                    text: qsTr("Statistics")
                    color: Theme.text
                    font.pointSize: Theme.fontSize * 1.2
                    font.weight: Font.DemiBold
                }

                RowLayout {
                    spacing: 32

                    ColumnLayout {
                        Label {
                            text: qsTr("Since first")
                            color: Theme.muted
                        }

                        Label {
                            text: App.overallChange
                        }

                    }

                    ColumnLayout {
                        Label {
                            text: qsTr("Lowest")
                            color: Theme.muted
                        }

                        Label {
                            text: App.lowest
                        }

                    }

                    ColumnLayout {
                        Label {
                            text: qsTr("Highest")
                            color: Theme.muted
                        }

                        Label {
                            text: App.highest
                        }

                    }

                    ColumnLayout {
                        Label {
                            text: qsTr("Target")
                            color: Theme.muted
                        }

                        Label {
                            text: App.target ? App.target + " kg" : "—"
                        }

                    }

                }

            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                height: 1
                color: Theme.border
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 28
                Layout.rightMargin: 28
                spacing: 8

                Label {
                    text: qsTr("History")
                    color: Theme.text
                    font.pointSize: Theme.fontSize * 1.2
                    font.weight: Font.DemiBold
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Date")
                        Layout.fillWidth: true
                        color: Theme.muted
                    }

                    Label {
                        text: qsTr("Weight")
                        Layout.preferredWidth: 100
                        color: Theme.muted
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        text: qsTr("Change")
                        Layout.preferredWidth: 100
                        color: Theme.muted
                        horizontalAlignment: Text.AlignRight
                    }

                    Item {
                        Layout.preferredWidth: 90
                    }

                }

                ListView {
                    id: history

                    Layout.fillWidth: true
                    Layout.preferredHeight: 290
                    clip: true
                    model: App.history
                    boundsBehavior: Flickable.StopAtBounds
                    focus: true
                    Keys.onReturnPressed: window.selectedEdit()
                    Keys.onDeletePressed: window.selectedDelete()

                    delegate: Rectangle {
                        required property string entryDate
                        required property string weight
                        required property string change
                        required property int index

                        width: history.width
                        height: 42
                        color: ListView.isCurrentItem ? Theme.surface : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            spacing: 8

                            Label {
                                text: entryDate
                                Layout.fillWidth: true
                                leftPadding: 8
                                font.features: {
                                    "tnum": 1
                                }
                            }

                            Label {
                                text: weight
                                Layout.preferredWidth: 100
                                horizontalAlignment: Text.AlignRight
                                font.features: {
                                    "tnum": 1
                                }
                            }

                            Label {
                                text: change
                                Layout.preferredWidth: 100
                                horizontalAlignment: Text.AlignRight
                                font.features: {
                                    "tnum": 1
                                }
                            }

                            Button {
                                text: qsTr("Edit")
                                flat: true
                                Layout.preferredWidth: 90
                                onClicked: window.editEntry(App.historyDate(index))
                            }

                        }

                        MouseArea {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: parent.width - 90
                            onClicked: history.currentIndex = index
                            onDoubleClicked: window.editEntry(App.historyDate(index))
                        }

                    }

                }

                Label {
                    visible: App.history.rowCount() === 0
                    text: qsTr("No entries yet. Add your first weight above.")
                    color: Theme.muted
                }

            }

            Item {
                height: 20
            }

        }

    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")

            Action {
                text: qsTr("New entry\tCtrl+N")
                onTriggered: window.resetEntry()
            }

            Action {
                text: qsTr("Backup Data…\tCtrl+B")
                onTriggered: backupFile.open()
            }

            Action {
                text: qsTr("Restore Data…")
                onTriggered: restoreFile.open()
            }

            MenuSeparator {
            }

            Action {
                text: qsTr("Export CSV…")
                onTriggered: exportFile.open()
            }

            Action {
                text: qsTr("Import CSV…")
                onTriggered: importFile.open()
            }

            MenuSeparator {
            }

            Action {
                text: qsTr("Quit\tCtrl+Q")
                onTriggered: Qt.quit()
            }

        }

        Menu {
            title: qsTr("Edit")

            Action {
                text: qsTr("Edit selected\tCtrl+E")
                onTriggered: window.selectedEdit()
            }

            Action {
                text: qsTr("Delete selected")
                onTriggered: window.selectedDelete()
            }

            Action {
                text: qsTr("Set target…")
                onTriggered: {
                    targetField.text = App.target;
                    targetDialog.open();
                }
            }

        }

        Menu {
            title: qsTr("Help")

            Action {
                text: qsTr("About")
                onTriggered: aboutDialog.open()
            }

        }

    }

}
