#include "PrecompiledHeader.h"

#include "DisplaySettingsWidget.h"
#include "QtUtils.h"
#include "SettingWidgetBinder.h"
#include "SettingsDialog.h"
#include <QtWidgets/QMessageBox>

DisplaySettingsWidget::DisplaySettingsWidget(QWidget* parent, SettingsDialog* dialog) : QWidget(parent)
{
  m_ui.setupUi(this);

  SettingWidgetBinder::BindWidgetToEnumSetting(m_ui.aspectRatio, "EmuCore/GS", "AspectRatio",
                                               Pcsx2Config::GSOptions::AspectRatioNames, AspectRatioType::R4_3);
  SettingWidgetBinder::BindWidgetToEnumSetting(m_ui.fmvAspectRatio, "EmuCore/GS", "FMVAspectRatioSwitch",
                                               Pcsx2Config::GSOptions::FMVAspectRatioSwitchNames,
                                               FMVAspectRatioSwitchType::Off);
  SettingWidgetBinder::BindWidgetToFloatSetting(m_ui.zoom, "EmuCore/GS", "Zoom", 100.0f);
  SettingWidgetBinder::BindWidgetToIntSetting(m_ui.offsetX, "EmuCore/GS", "OffsetX", 0);
  SettingWidgetBinder::BindWidgetToIntSetting(m_ui.offsetY, "EmuCore/GS", "OffsetY", 0);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.bilinearFiltering, "EmuCore/GS", "LinearPresent", true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.integerScaling, "EmuCore/GS", "IntegerScaling", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.startFullscreen, "UI", "StartFullscreen", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.doubleClickTogglesFullscreen, "UI", "DoubleClickTogglesFullscreen",
                                               true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.hideMouseCursor, "UI", "HideMouseCursor", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.renderToMainWindow, "UI", "RenderToMainWindow", true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.internalResolutionScreenshots, "EmuCore/GS",
                                               "InternalResolutionScreenshots", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.useBlitSwapChain, "EmuCore/GS", "UseBlitSwapChain", false);

  SettingWidgetBinder::BindWidgetToFloatSetting(m_ui.osdScale, "EmuCore/GS", "OsdScale", 100.0f);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.osdShowMessages, "EmuCore/GS", "OsdShowMessages", true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.osdShowSpeed, "EmuCore/GS", "OsdShowSpeed", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.osdShowFPS, "EmuCore/GS", "OsdShowFPS", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.osdShowCPU, "EmuCore/GS", "OsdShowCPU", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.osdShowResolution, "EmuCore/GS", "OsdShowResolution", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_ui.osdShowGSStats, "EmuCore/GS", "OsdShowGSStats", false);

  connect(m_ui.fullscreenModes, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &DisplaySettingsWidget::onFullscreenModeChanged);
  connect(m_ui.integerScaling, &QCheckBox::stateChanged, this, &DisplaySettingsWidget::onIntegerScalingChanged);
  onIntegerScalingChanged();

  dialog->registerWidgetHelp(m_ui.useBlitSwapChain, tr("Use Blit Swap Chain"), tr("Unchecked"),
                             tr("Uses a blit presentation model instead of flipping when using the Direct3D 11 "
                                "renderer. This usually results in slower performance, but may be required for some "
                                "streaming applications, or to uncap framerates on some systems."));

  dialog->registerWidgetHelp(m_ui.osdShowMessages, tr("Show OSD Messages"), tr("Checked"),
                             tr("Shows on-screen-display messages when events occur such as save states being "
                                "created/loaded, screenshots being taken, etc."));
  dialog->registerWidgetHelp(m_ui.osdShowFPS, tr("Show Game Frame Rate"), tr("Unchecked"),
                             tr("Shows the internal frame rate of the game in the top-right corner of the display."));
  dialog->registerWidgetHelp(
    m_ui.osdShowSpeed, tr("Show Emulation Speed"), tr("Unchecked"),
    tr("Shows the current emulation speed of the system in the top-right corner of the display as a percentage."));
  dialog->registerWidgetHelp(m_ui.osdShowResolution, tr("Show Resolution"), tr("Unchecked"),
                             tr("Shows the resolution of the game in the top-right corner of the display."));
  dialog->registerWidgetHelp(m_ui.startFullscreen, tr("Start Fullscreen"), tr("Unchecked"),
                             tr("Automatically switches to fullscreen mode when a game is started."));
  dialog->registerWidgetHelp(m_ui.hideMouseCursor, tr("Hide Cursor In Fullscreen"), tr("Checked"),
                             tr("Hides the mouse pointer/cursor when the emulator is in fullscreen mode."));
  dialog->registerWidgetHelp(
    m_ui.renderToMainWindow, tr("Render To Main Window"), tr("Checked"),
    tr("Renders the display of the simulated console to the main window of the application, over "
       "the game list. If unchecked, the display will render in a separate window."));
}

DisplaySettingsWidget::~DisplaySettingsWidget() = default;

void DisplaySettingsWidget::onFullscreenModesChanged(const QStringList& modes)
{
  QSignalBlocker sb(m_ui.fullscreenModes);

  const QString current_mode(
    QString::fromStdString(QtHost::GetBaseStringSettingValue("EmuCore/GS", "FullscreenMode", "")));
  m_ui.fullscreenModes->clear();
  m_ui.fullscreenModes->addItem(tr("Borderless Fullscreen"));
  if (current_mode.isEmpty())
    m_ui.fullscreenModes->setCurrentIndex(0);

  for (const QString& mode : modes)
  {
    m_ui.fullscreenModes->addItem(mode);
    if (current_mode == mode)
      m_ui.fullscreenModes->setCurrentIndex(m_ui.fullscreenModes->count() - 1);
  }
}

void DisplaySettingsWidget::onFullscreenModeChanged(int index)
{
  if (index == 0)
  {
    QtHost::RemoveBaseSettingValue("EmuCore/GS", "FullscreenMode");
  }
  else
  {
    QtHost::SetBaseStringSettingValue("EmuCore/GS", "FullscreenMode",
                                      m_ui.fullscreenModes->currentText().toUtf8().constData());
  }

  g_emu_thread->applySettings();
}

void DisplaySettingsWidget::onIntegerScalingChanged()
{
  m_ui.bilinearFiltering->setEnabled(!m_ui.integerScaling->isChecked());
}
