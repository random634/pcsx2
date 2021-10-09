#include "PrecompiledHeader.h"

#include "InterfaceSettingsWidget.h"
#include "SettingWidgetBinder.h"
#include "SettingsDialog.h"

InterfaceSettingsWidget::InterfaceSettingsWidget(QWidget* parent, SettingsDialog* dialog) : QWidget(parent)
{
  m_ui.setupUi(this);

  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.confirmPowerOff, "UI", "ConfirmPowerOff", true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.saveStateOnExit, "EmuCore", "AutoStateLoadSave", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.pauseOnStart, "UI", "StartPaused", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.pauseOnFocusLoss, "UI", "PauseOnFocusLoss", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.inhibitScreensaver, "UI", "InhibitScreensaver", true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.discordPresence, "UI", "DiscordPresence", false);

  dialog->registerWidgetHelp(
    m_ui.confirmPowerOff, tr("Confirm Power Off"), tr("Checked"),
    tr("Determines whether a prompt will be displayed to confirm shutting down the emulator/game "
       "when the hotkey is pressed."));
  dialog->registerWidgetHelp(m_ui.saveStateOnExit, tr("Save State On Exit"), tr("Checked"),
                             tr("Automatically saves the emulator state when powering down or exiting. You can then "
                                "resume directly from where you left off next time."));
  dialog->registerWidgetHelp(m_ui.pauseOnStart, tr("Pause On Start"), tr("Unchecked"),
                             tr("Pauses the emulator when a game is started."));
  dialog->registerWidgetHelp(m_ui.pauseOnFocusLoss, tr("Pause On Focus Loss"), tr("Unchecked"),
                             tr("Pauses the emulator when you minimize the window or switch to another application, "
                                "and unpauses when you switch back."));
  dialog->registerWidgetHelp(
    m_ui.inhibitScreensaver, tr("Inhibit Screensaver"), tr("Checked"),
    tr("Prevents the screen saver from activating and the host from sleeping while emulation is running."));

  dialog->registerWidgetHelp(m_ui.discordPresence, tr("Enable Discord Presence"), tr("Unchecked"),
                             tr("Shows the game you are currently playing as part of your profile in Discord."));
  if (true)
  {
    SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.autoUpdateEnabled, "AutoUpdater", "CheckAtStartup", true);
    dialog->registerWidgetHelp(m_ui.autoUpdateEnabled, tr("Enable Automatic Update Check"), tr("Checked"),
                               tr("Automatically checks for updates to the program on startup. Updates can be deferred "
                                  "until later or skipped entirely."));

    // m_ui.autoUpdateTag->addItems(AutoUpdaterDialog::getTagList());
    // SettingWidgetBinder::BindWidgetToStringSetting(m_ui.autoUpdateTag, "AutoUpdater", "UpdateTag",
    // AutoUpdaterDialog::getDefaultTag());

    // m_ui.autoUpdateCurrentVersion->setText(tr("%1 (%2)").arg(g_scm_tag_str).arg(g_scm_date_str));
    // connect(m_ui.checkForUpdates, &QPushButton::clicked, [this]() {
    // m_host_interface->getMainWindow()->checkForUpdates(true); });
  }
  else
  {
    m_ui.verticalLayout->removeWidget(m_ui.automaticUpdaterGroup);
    m_ui.automaticUpdaterGroup->hide();
  }
}

InterfaceSettingsWidget::~InterfaceSettingsWidget() = default;
