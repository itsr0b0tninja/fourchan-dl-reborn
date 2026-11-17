#include "uiconfig.h"
#include "fourchanrequestheaders.h"
#include "ui_uiconfig.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractSpinBox>
#include <QMessageBox>
#include "apptheme.h"

UIConfig::UIConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UIConfig)
{
    ui->setupUi(this);

    // Light and Dark share the same Qt Fusion control style.
    themeSelector = new QComboBox(this);
    themeSelector->setObjectName(QStringLiteral("themeSelector"));
    themeSelector->addItem(tr("Light"), QStringLiteral("light"));
    themeSelector->addItem(tr("Dark"), QStringLiteral("dark"));
    themeSelector->setToolTip(tr("Changes apply when you save Settings, without restarting."));
    ui->formLayout_3->insertRow(1, tr("Theme"), themeSelector);

    downloadVideosCheckBox = new QCheckBox(tr("Download MP4 and WebM videos by default"), this);
    downloadVideosCheckBox->setToolTip(tr("Individual threads can override this setting beside their rescan options."));
    ui->formLayout->addRow(tr("Videos"), downloadVideosCheckBox);

    // One global destination root. Individual thread DLL placeholders such
    // as %b/%n remain intact when its files are moved.
    moveThreadFolderEdit = new QLineEdit(this);
    moveThreadFolderEdit->setPlaceholderText(tr("Choose a destination root folder"));
    moveThreadFolderEdit->setToolTip(tr("The thread's board/thread subfolders will be preserved automatically."));
    QPushButton* chooseMoveFolderButton = new QPushButton(tr("Browse..."), this);
    QHBoxLayout* moveFolderRow = new QHBoxLayout();
    moveFolderRow->addWidget(moveThreadFolderEdit);
    moveFolderRow->addWidget(chooseMoveFolderButton);
    ui->formLayout_4->insertRow(1, tr("Move threads to"), moveFolderRow);
    connect(chooseMoveFolderButton, SIGNAL(clicked()), this, SLOT(chooseMoveThreadLocation()));

    // Small update-check section in General; the existing update action and
    // version-information dialog remain unchanged.
    QGroupBox* updateGroup = new QGroupBox(tr("Update checking"), this);
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    updateStartupCheckBox = new QCheckBox(tr("Check for updates on startup"), updateGroup);
    updatePeriodicCheckBox = new QCheckBox(tr("Check every"), updateGroup);
    updateIntervalDaysSpinBox = new QSpinBox(updateGroup);
    updateIntervalDaysSpinBox->setRange(1, 365);
    updateIntervalDaysSpinBox->setSuffix(tr(" days"));
    updateIntervalDaysSpinBox->setToolTip(tr("Session-based interval; starts again whenever the application opens."));
    updateNotifyCheckBox = new QCheckBox(tr("Notify when a new version is available"), updateGroup);
    QHBoxLayout* intervalRow = new QHBoxLayout();
    intervalRow->addWidget(updatePeriodicCheckBox);
    intervalRow->addWidget(updateIntervalDaysSpinBox);
    intervalRow->addStretch();
    updateLayout->addWidget(updateStartupCheckBox);
    updateLayout->addLayout(intervalRow);
    updateLayout->addWidget(updateNotifyCheckBox);
    ui->formLayout_3->addRow(updateGroup);
    connect(updatePeriodicCheckBox, SIGNAL(toggled(bool)), updateIntervalDaysSpinBox, SLOT(setEnabled(bool)));

    _removing_thumbnails = false;
    loadingSettings = false;

    settings = new QSettings("settings.ini", QSettings::IniFormat);
    timeoutValueEditor = new UIListEditor(this);
    timeoutValueEditor->setModal(true);

    dialogFolderShortcut = new DialogFolderShortcut(this);
    dialogFolderShortcut->setModal(true);

    userAgentStrings.insert("Wget", "Wget/1.12");
    userAgentStrings.insert("Opera", "Opera/9.80 (Windows NT 6.0) Presto/2.12.388 Version/12.14");
    userAgentStrings.insert("Firefox", "Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:25.0) Gecko/20100101 Firefox/25.0");
    userAgentStrings.insert("Chrome", "Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1667.0 Safari/537.36");

    loadSettings();
    loadShortcuts();
    refreshThemeWarning();

    // Persist navigation immediately without marking ordinary preferences dirty.
    const int lastTab = settings->value("ui/configuration_tab", 0).toInt();
    ui->tabWidget->setCurrentIndex(qBound(0, lastTab, ui->tabWidget->count() - 1));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(rememberSettingsTab(int)));
    connectApplyTracking();
    connect(ui->btnApply, SIGNAL(clicked()), this, SLOT(applySettings()));

    // The timeout subdialog saves its own values, without overwriting other
    // pending edits in this Settings window.
    connect(timeoutValueEditor, SIGNAL(valuesChanged()),
            this, SLOT(timeoutValuesChanged()));
    connect(ui->cbUseProxy, SIGNAL(toggled(bool)), this, SLOT(toggleProxy(bool)));
    connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(editShortcutItem(QListWidgetItem*)));
    connect(folderShortcuts, SIGNAL(shortcutsChanged()), this, SLOT(loadShortcuts()));
    connect(dialogFolderShortcut, SIGNAL(shortcutChanged(QString,QString,QString)), folderShortcuts, SLOT(updateShortcut(QString,QString,QString)));
    connect(dialogFolderShortcut, SIGNAL(editCanceled()), this, SLOT(loadShortcuts()));
    connect(ui->btnDeleteAllThumbnails, SIGNAL(clicked()), this, SIGNAL(deleteAllThumbnails()));
    connect(ui->btnDeleteAllThumbnails, SIGNAL(clicked()), this, SLOT(thumbnailDeletionStarted()));
}

UIConfig::~UIConfig()
{
    delete ui;
}

void UIConfig::loadSettings(void) {
    loadingSettings = true;
    settings->sync();
    bool b;

    settings->beginGroup("updater");
    updateStartupCheckBox->setChecked(settings->value("check_on_startup", true).toBool());
    updatePeriodicCheckBox->setChecked(settings->value("check_periodic", true).toBool());
    updateIntervalDaysSpinBox->setValue(qBound(1, settings->value("check_interval_days", 1).toInt(), 365));
    updateNotifyCheckBox->setChecked(settings->value("notify_new_version", true).toBool());
    settings->endGroup();

    settings->beginGroup("options");
    const QString themeName = settings->value("theme", "light").toString().trimmed().toLower();
    const int themeIndex = themeSelector->findData(themeName);
    themeSelector->setCurrentIndex(themeIndex < 0 ? 0 : themeIndex);
    ui->leDefaultSavepath->setText(settings->value("default_directory","").toString());
    moveThreadFolderEdit->setText(settings->value("move_thread_directory", "").toString());
    ui->cmbTabPosition->setCurrentIndex(settings->value("tab_position",3).toInt());
    b = settings->value("automatic_close",false).toBool();
        ui->cbAutoClose->setChecked(b);

    b = settings->value("resume_session",true).toBool();
        ui->cbReopenTabs->setChecked(b);

    b = settings->value("enlarge_thumbnails",false).toBool();
        ui->cbEnlargeThumbnails->setChecked(b);

    b = settings->value("hq_thumbnails",true).toBool();
        ui->cbHQThumbnail->setChecked(b);

    b = settings->value("default_original_filename",false).toBool();
        ui->cbDefaultOriginalFilename->setChecked(b);

    b = settings->value("remember_directory",true).toBool();
        ui->cbRememberDirectory->setChecked(b);

    ui->cbCloseOverviewThreads->setChecked(settings->value("close_overview_threads", true).toBool());
    ui->cbUseInternalViewer->setChecked(settings->value("use_internal_viewer", true).toBool());

    ui->sbConcurrentDownloads->setValue(settings->value("concurrent_downloads",1).toInt());
    ui->sbRescheduleInterval->setValue(settings->value("reschedule_interval", 60).toInt());
    ui->sbThumbnailHeight->setValue(settings->value("thumbnail_height",200).toInt());
    ui->sbThumbnailWidth->setValue(settings->value("thumbnail_width",200).toInt());
    ui->leThumbnailCacheFolder->setText(settings->value("thumbnail_cache_folder", QString("%1/%2").arg(QCoreApplication::applicationDirPath())
                                                        .arg("tncache")).toString());
    ui->sbThumbnailTTL->setValue(settings->value("thumbnail_TTL", 60).toInt());

    rebuildRescanIntervals(settings->value("default_timeout", 0).toInt());

    ui->cbCloseToTray->setChecked(settings->value("close_to_tray", false).toBool());
    ui->cbLoggingLevel->setCurrentIndex(settings->value("log_level",3).toInt());
    settings->endGroup();

    settings->beginGroup("blacklist");
    b = settings->value("use_blacklist",true).toBool();
    ui->cbUseBlackList->setChecked(b);
    ui->sbBlackListCheckInterval->setValue(settings->value("blacklist_check_interval", 600).toInt());
    settings->endGroup();

    settings->beginGroup("network");
        b = settings->value("use_proxy", false).toBool();
        ui->cbUseProxy->setChecked(b);
        ui->cbProxyAuth->setChecked(settings->value("proxy_auth", false).toBool());
        toggleProxy(b);

        ui->leProxyAddress->setText(settings->value("proxy_hostname", "").toString());
        ui->sbProxyPort->setValue(settings->value("proxy_port", 0).toInt());
        ui->leProxyUser->setText(settings->value("proxy_user", "").toString());
        ui->leProxyPassword->setText(settings->value("proxy_pass", "").toString());

        int proxyType;
        proxyType = settings->value("proxy_type", 0).toInt();
        if (proxyType>=2) proxyType--;
        ui->cbProxyType->setCurrentIndex(proxyType);
    settings->endGroup();

    settings->beginGroup("download_manager");
        downloadVideosCheckBox->setChecked(settings->value("download_videos", true).toBool());
        ui->sbConcurrentDownloads->setValue(settings->value("concurrent_downloads",20).toInt());
        ui->sbDownloadTimeoutInitial->setValue(settings->value("initial_timeout",30).toInt());
        ui->sbDownloadTimeoutInbetween->setValue(settings->value("running_timeout",20).toInt());
        ui->sbMaxRetries->setValue(settings->value("max_retries",4).toInt());
        ui->sbRetryDelay->setValue(settings->value("retry_delay_seconds",5).toInt());
        ui->cbKeepLocalHTMLCopy->setChecked(settings->value("use_thread_cache", false).toBool());
        ui->leThreadCachePath->setText(settings->value("thread_cache_path", QString("%1/%2").arg(QCoreApplication::applicationDirPath())
                                                       .arg("thread-cache")).toString());
        ui->cbCompressCacheFile->setChecked(settings->value("compress_cache_file", true).toBool());

        QString agent = settings->value("user-agent",
            QString::fromLatin1(FourchanRequestHeaders::defaultUserAgent())).toString();
        if (agent.trimmed().isEmpty() || agent == "Wget/1.12")
            agent = QString::fromLatin1(FourchanRequestHeaders::defaultUserAgent());
        ui->leUserAgent->setText(agent);
        // Avoid duplicated presets and overwriting a saved custom User-Agent.
        const bool wasBlocked = ui->cmbUserAgent->blockSignals(true);
        ui->cmbUserAgent->clear();
        ui->cmbUserAgent->addItems(userAgentStrings.keys());
        ui->cmbUserAgent->blockSignals(wasBlocked);
    settings->endGroup();

    ui->sbUpdaterPort->setValue(settings->value("updater/updater_port", 60000).toInt());
    ui->sbApplicationPort->setValue(settings->value("updater/application_port", 60001).toInt());

    timeoutValueEditor->loadSettings();
    loadingSettings = false;
    captureAppliedValues();
}

void UIConfig::rebuildRescanIntervals(int selectedSeconds) {
    const QStringList sl = settings->value(
        "options/timeout_values", (QStringList() << "300" << "600")).toStringList();
    ui->cbRescanInterval->clear();
    ui->cbRescanInterval->addItem(tr("Never"), 0);
    foreach (const QString &s, sl) {
        bool ok = false;
        const int seconds = s.toInt(&ok);
        if (!ok)
            continue;
        int value = seconds;
        QString unit = tr("seconds");
        if (seconds > 60 && seconds % 60 == 0) {
            if (seconds > 3600 && seconds % 3600 == 0) {
                if (seconds > 86400 && seconds % 86400 == 0) {
                    value = seconds / 86400;
                    unit = tr("days");
                } else {
                    value = seconds / 3600;
                    unit = tr("hours");
                }
            } else {
                value = seconds / 60;
                unit = tr("minutes");
            }
        }
        ui->cbRescanInterval->addItem(tr("every %1 %2").arg(value).arg(unit), seconds);
    }
    const int match = ui->cbRescanInterval->findData(selectedSeconds);
    ui->cbRescanInterval->setCurrentIndex(match >= 0 ? match : 0);
}

void UIConfig::timeoutValuesChanged() {
    // The separate editor already persisted its interval list. Preserve all
    // other unsaved Settings edits when refreshing just this dropdown.
    const int selectedSeconds = ui->cbRescanInterval->currentData().toInt();
    loadingSettings = true;
    rebuildRescanIntervals(selectedSeconds);
    loadingSettings = false;
    updateApplyState();
}

void UIConfig::rememberSettingsTab(int index) {
    settings->setValue("ui/configuration_tab", index);
    settings->sync();
}

QMap<QWidget*, QVariant> UIConfig::readCurrentValues() const {
    // Ignore the independently saved shortcut/timeout subdialogs.
    QMap<QWidget*, QVariant> values;
    const QList<QCheckBox*> checks = ui->tabWidget->findChildren<QCheckBox*>();
    foreach (QCheckBox *widget, checks)
        values.insert(widget, widget->isChecked());
    const QList<QComboBox*> combos = ui->tabWidget->findChildren<QComboBox*>();
    foreach (QComboBox *widget, combos) {
        // The User-Agent preset picker is only a helper; leUserAgent is saved.
        if (widget != ui->cmbUserAgent)
            values.insert(widget, widget->currentData().isValid()
                                      ? widget->currentData() : QVariant(widget->currentIndex()));
    }
    const QList<QSpinBox*> spins = ui->tabWidget->findChildren<QSpinBox*>();
    foreach (QSpinBox *widget, spins)
        values.insert(widget, widget->value());
    const QList<QLineEdit*> edits = ui->tabWidget->findChildren<QLineEdit*>();
    foreach (QLineEdit *widget, edits) {
        if (qobject_cast<QAbstractSpinBox*>(widget->parentWidget()) ||
            qobject_cast<QComboBox*>(widget->parentWidget()))
            continue;
        values.insert(widget, widget->text());
    }
    return values;
}

void UIConfig::captureAppliedValues() {
    appliedValues = readCurrentValues();
    ui->btnApply->setEnabled(false);
}

void UIConfig::updateApplyState() {
    if (!loadingSettings)
        ui->btnApply->setEnabled(readCurrentValues() != appliedValues);
}

void UIConfig::connectApplyTracking() {
    const QList<QCheckBox*> checks = ui->tabWidget->findChildren<QCheckBox*>();
    foreach (QCheckBox *widget, checks)
        connect(widget, SIGNAL(toggled(bool)), this, SLOT(updateApplyState()));
    const QList<QComboBox*> combos = ui->tabWidget->findChildren<QComboBox*>();
    foreach (QComboBox *widget, combos)
        connect(widget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateApplyState()));
    const QList<QSpinBox*> spins = ui->tabWidget->findChildren<QSpinBox*>();
    foreach (QSpinBox *widget, spins)
        connect(widget, SIGNAL(valueChanged(int)), this, SLOT(updateApplyState()));
    const QList<QLineEdit*> edits = ui->tabWidget->findChildren<QLineEdit*>();
    foreach (QLineEdit *widget, edits)
        connect(widget, SIGNAL(textChanged(QString)), this, SLOT(updateApplyState()));
}

void UIConfig::applySettings() {
    saveConfiguration();
}

void UIConfig::accept(void) {
    if (saveConfiguration())
        hide();
}

bool UIConfig::saveConfiguration() {
    settings->beginGroup("updater");
        settings->setValue("check_on_startup", updateStartupCheckBox->isChecked());
        settings->setValue("check_periodic", updatePeriodicCheckBox->isChecked());
        settings->setValue("check_interval_days", updateIntervalDaysSpinBox->value());
        settings->setValue("notify_new_version", updateNotifyCheckBox->isChecked());
    settings->endGroup();

    settings->beginGroup("options");
        settings->setValue("default_directory", ui->leDefaultSavepath->text());
        settings->setValue("theme", themeSelector->itemData(themeSelector->currentIndex()).toString());
        settings->setValue("move_thread_directory", moveThreadFolderEdit->text().trimmed());
        settings->setValue("tab_position", ui->cmbTabPosition->currentIndex());

        settings->setValue("automatic_close", ui->cbAutoClose->isChecked());
        settings->setValue("resume_session", ui->cbReopenTabs->isChecked());
        settings->setValue("enlarge_thumbnails", ui->cbEnlargeThumbnails->isChecked());
        settings->setValue("hq_thumbnails", ui->cbHQThumbnail->isChecked());
        settings->setValue("default_original_filename", ui->cbDefaultOriginalFilename->isChecked());
        settings->setValue("remember_directory", ui->cbRememberDirectory->isChecked());

        settings->setValue("concurrent_downloads", ui->sbConcurrentDownloads->value());
        settings->setValue("reschedule_interval", ui->sbRescheduleInterval->value());
        settings->setValue("thumbnail_width", ui->sbThumbnailWidth->value());
        settings->setValue("thumbnail_height", ui->sbThumbnailHeight->value());

        settings->setValue("default_timeout", ui->cbRescanInterval->itemData(ui->cbRescanInterval->currentIndex()));

        settings->setValue("thumbnail_cache_folder", ui->leThumbnailCacheFolder->text());
        settings->setValue("thumbnail_TTL", ui->sbThumbnailTTL->value());

        settings->setValue("close_overview_threads", ui->cbCloseOverviewThreads->isChecked());
        settings->setValue("use_internal_viewer", ui->cbUseInternalViewer->isChecked());
        settings->setValue("close_to_tray", ui->cbCloseToTray->isChecked());

        settings->setValue("log_level", ui->cbLoggingLevel->currentIndex());

    settings->endGroup();
    settings->beginGroup("blacklist");
        if (ui->cbUseBlackList->isChecked())
            settings->setValue("use_blacklist", true);
        else
            settings->setValue("use_blacklist", false);

        settings->setValue("blacklist_check_interval", ui->sbBlackListCheckInterval->value());
    settings->endGroup();

    settings->beginGroup("network");
        settings->setValue("use_proxy", ui->cbUseProxy->isChecked());
        settings->setValue("proxy_auth", ui->cbProxyAuth->isChecked());
        settings->setValue("proxy_hostname",  ui->leProxyAddress->text());
        settings->setValue("proxy_port",  ui->sbProxyPort->value());
        settings->setValue("proxy_user",  ui->leProxyUser->text());
        settings->setValue("proxy_pass",  ui->leProxyPassword->text());

        int proxyType;

        proxyType = ui->cbProxyType->currentIndex();
        if (proxyType >= 2) proxyType++;

        settings->setValue("proxy_type", proxyType);
    settings->endGroup();

    settings->beginGroup("download_manager");
        settings->setValue("download_videos", downloadVideosCheckBox->isChecked());
        settings->setValue("concurrent_downloads", ui->sbConcurrentDownloads->value());
        settings->setValue("initial_timeout", ui->sbDownloadTimeoutInitial->value());
        settings->setValue("running_timeout", ui->sbDownloadTimeoutInbetween->value());
        settings->setValue("max_retries", ui->sbMaxRetries->value());
        settings->setValue("retry_delay_seconds", ui->sbRetryDelay->value());
        settings->setValue("use_thread_cache", ui->cbKeepLocalHTMLCopy->isChecked());
        settings->setValue("thread_cache_path", ui->leThreadCachePath->text());
        settings->setValue("user-agent", ui->leUserAgent->text());
        settings->setValue("compress_cache_file", ui->cbCompressCacheFile->isChecked());
    settings->endGroup();

    settings->setValue("updater/updater_port", ui->sbUpdaterPort->value());
    settings->setValue("updater/application_port", ui->sbApplicationPort->value());

    settings->sync();
    if (settings->status() != QSettings::NoError) {
        QMessageBox::warning(this, tr("Settings could not be saved"),
                             tr("Changes could not be written to settings.ini. "
                                "Please check folder permissions and try again."));
        return false;
    }

    captureAppliedValues();
    emit configurationChanged();
    return true;
}

void UIConfig::reject(void) {
    hide();
    // Cancel discards edits made since the last successful Apply/OK.
    loadSettings();
    loadShortcuts();
}

void UIConfig::chooseLocation(void) {
    QString loc;

    loc = QFileDialog::getExistingDirectory(this, "Choose storage directory", ui->leDefaultSavepath->text());
    if (!loc.isEmpty())
        ui->leDefaultSavepath->setText(loc);
}

void UIConfig::chooseMoveThreadLocation(void) {
    const QString location = QFileDialog::getExistingDirectory(
                this, tr("Choose folder for moved threads"), moveThreadFolderEdit->text());
    if (!location.isEmpty())
        moveThreadFolderEdit->setText(QDir::toNativeSeparators(location));
}

void UIConfig::chooseThumbnailCacheLocation(void) {
    QString loc;

    loc = QFileDialog::getExistingDirectory(this, "Choose thumbnail cache directory", ui->leThumbnailCacheFolder->text());
    if (!loc.isEmpty())
        ui->leThumbnailCacheFolder->setText(loc);
}

void UIConfig::chooseThreadCacheLocation(void) {
    QString loc;

    loc = QFileDialog::getExistingDirectory(this, "Choose thread cache directory", ui->leThreadCachePath->text());
    if (!loc.isEmpty())
        ui->leThreadCachePath->setText(loc);
}

void UIConfig::editTimeoutValues(void) {
    timeoutValueEditor->show();
}

void UIConfig::toggleProxy(bool b) {
    ui->leProxyAddress->setEnabled(b);
    ui->sbProxyPort->setEnabled(b);
    ui->cbProxyType->setEnabled(b);
    ui->cbProxyAuth->setEnabled(b);

    if (b) {
        if (ui->cbProxyAuth->isChecked()) {
            ui->leProxyUser->setEnabled(b);
            ui->leProxyPassword->setEnabled(b);
        }
    }
    else {
        ui->leProxyUser->setEnabled(b);
        ui->leProxyPassword->setEnabled(b);
    }
}

void UIConfig::addShortcut() {
//    ui->listWidget->addItem("???");
//    editShortcutItem(ui->listWidget->item(ui->listWidget->count()-1));
    editShortcut("");
}

void UIConfig::editShortcut(QString name) {
    dialogFolderShortcut->clear();
    dialogFolderShortcut->edit(name);

    dialogFolderShortcut->show();
}

void UIConfig::editShortcutItem(QListWidgetItem* item) {
    editShortcut(item->text());
}

void UIConfig::loadShortcuts() {
    ui->listWidget->clear();

    ui->listWidget->addItems(folderShortcuts->shortcuts());
}

void UIConfig::deleteShortcut() {
    QString name;

    name = ui->listWidget->currentItem()->text();

    if (name != "???")
        folderShortcuts->deleteShortcut(name);
    else
        ui->listWidget->removeItemWidget(ui->listWidget->currentItem());
}

void UIConfig::refreshThemeWarning() {
    // The original .ui embeds dark-red rich text, which is difficult to read
    // against charcoal. Keep the wording and visibility behaviour intact.
    ui->lLogLevelWarning->setText(
        QStringLiteral("<span style=\"font-weight:600;color:%1;\">"
            "Warning: Setting logging level to &lt;Trace&gt; may/will slow down "
            "the program and generate a huge logfile</span>")
        .arg(AppTheme::warningTextColor()));
}

void UIConfig::toggleLogLevelWarning(QString s) {
    if (s == "Trace") {
        ui->lLogLevelWarning->show();
    }
    else {
        ui->lLogLevelWarning->hide();
    }
}

void UIConfig::thumbnailDeletionFinished() {
    if (_removing_thumbnails) {
        ui->lDeleteThumbnailsStatus->setText("Deleting finished");
        _removing_thumbnails = false;
    }
}

void UIConfig::thumbnailDeletionStarted() {
    ui->lDeleteThumbnailsStatus->setText("Deleting");
    _removing_thumbnails = true;
}

void UIConfig::setUserAgentString() {
    if (ui->cmbUserAgent->currentText() != "---") {
        ui->leUserAgent->setText(userAgentStrings.value(ui->cmbUserAgent->currentText()));
    }
}
