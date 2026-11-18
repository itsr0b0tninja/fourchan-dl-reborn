#include "uiinfo.h"
#include "ui_uiinfo.h"
#include "componenttreelayout.h"
#include <QFontDatabase>
#include <QScrollBar>
#include <QResizeEvent>
#include <QShowEvent>
UIInfo::UIInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UIInfo),
    latestAvailableVersion(tr("Not checked")),
    uiReady(false)
{
    ui->setupUi(this);
    uiReady = true;
    // The Information tab is the first view on the first opening. The dialog
    // stays alive in MainWindow, so switching tabs/expanding component branches
    // remains intact throughout the running application session.
    ui->tabWidget->setCurrentIndex(0);
    ui->label_2->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label_2->setOpenExternalLinks(true);
    ui->textEdit->setReadOnly(true);
    ui->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    ui->textEdit->setMaximumBlockCount(10000);
    ui->textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    uiPendingRequests = new UIPendingRequests(this);
    timer = new QTimer(this);
    timer->setInterval(3000);
    timer->setSingleShot(false);
    timer->start();

    // All tree entries and values retain their original nesting.
    // Changing tabs or reopening the window must not collapse branches.
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 1)
            resizeComponentColumns();
    });

    updateAboutText();
    updateStatistics();
    updateDebugInformation();

    connect(timer, SIGNAL(timeout()), this, SLOT(updateStatistics()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateDebugInformation()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateLogFile()));
    connect(ui->btnShowRequests, SIGNAL(clicked()), this, SLOT(showRequests()));
    connect(uiPendingRequests, SIGNAL(reloadRequested()), this, SLOT(reloadRequests()));
    connect(this, SIGNAL(rejected()), uiPendingRequests, SLOT(hide()));

    logFile = new QFile(QStringLiteral("fourchan-dl.log"), this);
    logFile->open(QIODevice::ReadOnly | QIODevice::Unbuffered | QIODevice::Text);
}

void UIInfo::setCurrentVersion(QString version) {
    latestAvailableVersion = version.trimmed().isEmpty() ? tr("Not checked") : version.trimmed();
    updateAboutText();
}

void UIInfo::updateAboutText() {
#ifdef __DEBUG__
    const QString installedVersion = QStringLiteral("DEBUG");
#else
    const QString installedVersion = QString::fromLatin1(PROGRAM_VERSION);
#endif
    // Use inherited palette colours rather than hard-coded black or blue,
    // so this information is readable under both Fusion Light and Dark.
    const QString html = QStringLiteral(
        "<p style='margin-top:0px; margin-bottom:10px;'>"
        "<b>4chan Downloader Reborn</b></p>"
        "<p style='margin-top:0px; margin-bottom:10px;'>"
        "Installed version: <b>%1</b><br/>"
        "Latest available version: %2</p>"
        "<p style='margin-top:0px; margin-bottom:10px;'>"
        "Build date: %3<br/>"
        "Built with Qt: %4</p>"
        "<p style='margin-top:0px; margin-bottom:10px;'>"
        "This software is provided as-is, without warranty.<br/>"
        "Released under the GNU General Public License, version 3.</p>"
        "<p style='margin-top:0px; margin-bottom:8px;'>"
        "<a href='https://www.gnu.org/licenses/gpl-3.0.html'>Read the GPLv3 licence</a></p>"
        "<p style='margin-top:0px; margin-bottom:0px;'>"
        "<a href='https://github.com/itsr0b0tninja/fourchan-dl-reborn'>GitHub repository</a>"
        " &nbsp;|&nbsp; "
        "<a href='https://github.com/itsr0b0tninja/fourchan-dl-reborn/issues'>Report a bug</a>"
        "</p>");
    ui->label_2->setText(html.arg(installedVersion.toHtmlEscaped(),
                                     latestAvailableVersion.toHtmlEscaped(),
                                     QString::fromLatin1(BUILDDATE).toHtmlEscaped(),
                                     QString::fromLatin1(QT_VERSION_STR).toHtmlEscaped()));
}

void UIInfo::updateStatistics() {
    int files = 0;
    float kilobytes = 0.0f;
    downloadManager->getStatistics(&files, &kilobytes);
    const QStringList units = QStringList() << "KB" << "MB" << "GB" << "TB";
    int index = 0;
    while (kilobytes >= 1024.0f && index < units.size() - 1) {
        kilobytes /= 1024.0f;
        ++index;
    }
    ui->lStatistics->setText(
                tr("Downloaded\n%1 files\n%2 %3")
                .arg(files)
                .arg(QString::number(kilobytes, 'f', 1))
                .arg(units.at(index)));
}

void UIInfo::updateDebugInformation() {
    ui->lDebugInformation->setText(
                tr("Pending requests: %1     Running requests: %2")
                .arg(downloadManager->getPendingRequests())
                .arg(downloadManager->getRunningRequests()));
}

void UIInfo::resizeComponentColumns() {
    // setupUi can resize the dialog before its child tree is constructed.
    if (uiReady)
        ComponentTreeLayout::sizeColumns(ui->pluginInfo);
}

void UIInfo::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    resizeComponentColumns();
}

void UIInfo::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    resizeComponentColumns();
    updateStatistics();
    updateDebugInformation();
    updateLogFile();
}

void UIInfo::loadComponentInfo(QMap<QString, component_information> components) {
    component_information c;
    ParserPluginInterface* p;
    QTreeWidgetItem* twi;
    QStringList keys;
    QMap<QString,QString> replacementStrings;

    // Restore expanded groups and plugin detail branches if a component list
    // is refreshed while the application remains open.
    const QSet<QString> expanded = ComponentTreeLayout::expandedPaths(ui->pluginInfo);
    ui->pluginInfo->setUpdatesEnabled(false);
    ui->pluginInfo->clear();
    twi = 0;
    keys = components.keys();
    QTreeWidgetItem* pluginTree = new QTreeWidgetItem(ui->pluginInfo);
    pluginTree->setText(0, "Plugins");
    QTreeWidgetItem* executableTree = new QTreeWidgetItem(ui->pluginInfo);
    executableTree->setText(0, "Executables");
    QTreeWidgetItem* qtTree = new QTreeWidgetItem(ui->pluginInfo);
    qtTree->setText(0, "Qt");
    QTreeWidgetItem* libTree = new QTreeWidgetItem(ui->pluginInfo);
    libTree->setText(0, "External libraries");
    QTreeWidgetItem* unknownTree = new QTreeWidgetItem(ui->pluginInfo);
    unknownTree->setText(0, "Not categorized");

    foreach (QString key, keys) {
        c = components.value(key);

        if (c.type.startsWith("plugin")) {
            p = pluginManager->getPlugin(key);

            if (p != 0) {
                QTreeWidgetItem* pluginName = new QTreeWidgetItem(pluginTree);
                pluginName->setText(0, p->getPluginName());

                QTreeWidgetItem* author = new QTreeWidgetItem(pluginName);
                author->setText(0, "Author");
                author->setText(1, p->getAuthor());

                QTreeWidgetItem* version = new QTreeWidgetItem(pluginName);
                version->setText(0, "Version");
                version->setText(1, p->getVersion());

                QTreeWidgetItem* iface = new QTreeWidgetItem(pluginName);
                iface->setText(0, "Interface Revision");
                iface->setText(1, p->getInterfaceRevision());

                QTreeWidgetItem* domain = new QTreeWidgetItem(pluginName);
                domain->setText(0, "Domain");
                domain->setText(1, p->getDomain());

                QTreeWidgetItem* replaceChars = new QTreeWidgetItem(pluginName);
                replaceChars->setText(0,"Available formatting");
                replacementStrings.clear();
                replacementStrings = p->getSupportedReplaceCharacters();

                QMapIterator<QString,QString> i(replacementStrings);
                while (i.hasNext()) {
                    i.next();
                    QTreeWidgetItem* rc_twi = new QTreeWidgetItem(replaceChars);
                    rc_twi->setText(0, i.key());
                    rc_twi->setText(1, i.value());
                }
            }
        }
        else {
            if (c.type.startsWith("executable")) {
                twi = executableTree;
            } else if (c.type.startsWith("qt")) {
                twi = qtTree;
            } else if (c.type.startsWith("library")) {
                twi = libTree;
            } else {
                twi = unknownTree;
            }

            QTreeWidgetItem* componentName = new QTreeWidgetItem(twi);
            componentName->setText(0, c.componentName);

            if (!c.version.isEmpty()) {
                QTreeWidgetItem* version = new QTreeWidgetItem(componentName);
                version->setText(0, "Version");
                version->setText(1, c.version);
            }
        }
    }

    ComponentTreeLayout::restoreExpandedPaths(ui->pluginInfo, expanded);
    ui->pluginInfo->setUpdatesEnabled(true);
    resizeComponentColumns();
}

void UIInfo::updateLogFile() {
    if (!logFile || !logFile->isReadable())
        return;

    // Keep indentation and exact log content. Never simplify whitespace or
    // parse log lines as rich text. Preserve the user's scroll position while
    // reviewing older diagnostics, and follow new lines only at the bottom.
    QStringList lines;
    while (!logFile->atEnd()) {
        QString line = QString::fromUtf8(logFile->readLine());
        while (line.endsWith(QLatin1Char('\n')) || line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        lines << line;
    }
    if (lines.isEmpty())
        return;

    QScrollBar *scroll = ui->textEdit->verticalScrollBar();
    const bool follow = scroll->value() >= scroll->maximum() - 2;
    const int position = scroll->value();
    ui->textEdit->appendPlainText(lines.join(QStringLiteral("\n")));
    scroll->setValue(follow ? scroll->maximum() : position);
}

UIInfo::~UIInfo()
{
    logFile->close();
    delete ui;
}

void UIInfo::showRequests() {
    reloadRequests();
    uiPendingRequests->show();
}

void UIInfo::reloadRequests() {
    uiPendingRequests->showPendingRequestList(downloadManager->getPendingRequestsMap());
}
