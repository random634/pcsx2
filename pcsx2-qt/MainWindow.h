#pragma once

#include <QtWidgets/QMainWindow>
#include <optional>

#include "SettingsDialog.h"
#include "ui_MainWindow.h"

class QProgressBar;

class DisplayWidget;
class DisplayContainer;
class GameListWidget;

class EmuThread;

namespace GameList {
struct Entry;
}

class MainWindow final : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(const QString& unthemed_style_name);
  ~MainWindow();

  void initialize();
  void connectVMThreadSignals(EmuThread* thread);

public Q_SLOTS:
  void refreshGameList(bool invalidate_cache);
  void invalidateSaveStateCache();

private Q_SLOTS:
  DisplayWidget* createDisplay(bool fullscreen, bool render_to_main);
  DisplayWidget* updateDisplay(bool fullscreen, bool render_to_main);
  void displaySizeRequested(qint32 width, qint32 height);
  void destroyDisplay();
  void focusDisplayWidget();

  void onGameListRefreshComplete();
  void onGameListRefreshProgress(const QString& status, int current, int total);
  void onGameListSelectionChanged();
  void onGameListEntryActivated();
  void onGameListEntryContextMenuRequested(const QPoint& point);

  void onStartFileActionTriggered();
  void onStartBIOSActionTriggered();
  void onLoadStateMenuAboutToShow();
  void onSaveStateMenuAboutToShow();
  void onViewToolbarActionToggled(bool checked);
  void onViewLockToolbarActionToggled(bool checked);
  void onViewStatusBarActionToggled(bool checked);
  void onViewGameListActionTriggered();
  void onViewGameGridActionTriggered();
  void onViewSystemDisplayTriggered();

  void onVMStarting();
  void onVMStarted();
  void onVMPaused();
  void onVMResumed();
  void onVMStopped();

  void onGameChanged(const QString& path, const QString& serial, const QString& name, quint32 crc);

protected:
  void closeEvent(QCloseEvent* event);

private:
  enum : s32
  {
    NUM_SAVE_STATE_SLOTS = 10,
  };

  void setupAdditionalUi();
  void connectSignals();
  void recreate();
  void setTheme(const char* theme);
  void setStyleFromSettings();
  void setIconThemeFromSettings();
  void addThemesToMenu();
  void updateMenuSelectedTheme();

  void saveStateToConfig();
  void restoreStateFromConfig();

  void updateEmulationActions(bool starting, bool running);
  void updateWindowTitle();
  void setProgressBar(int current, int total);
  void clearProgressBar();

  bool isShowingGameList() const;
  void switchToGameListView();
  void switchToEmulationView();

  QWidget* getDisplayContainer() const;
  void saveDisplayWindowGeometryToConfig();
  void restoreDisplayWindowGeometryFromConfig();
  void destroyDisplayWidget();
  void setDisplayFullscreen(const std::string& fullscreen_mode);

  SettingsDialog* getSettingsDialog();
  void doSettings(SettingsDialog::Category category = SettingsDialog::Category::Count);

  void startGameListEntry(const GameList::Entry* entry, std::optional<s32> save_slot, std::optional<bool> fast_boot);

  void loadSaveStateSlot(s32 slot);
  void populateLoadStateMenu(QMenu* menu, const QString& serial, quint32 crc);
  void populateSaveStateMenu(QMenu* menu, const QString& serial, quint32 crc);
  void updateSaveStateMenus(const QString& serial, quint32 crc);

  Ui::MainWindow m_ui;

  QString m_unthemed_style_name;

  GameListWidget* m_game_list_widget = nullptr;
  DisplayWidget* m_display_widget = nullptr;
  DisplayContainer* m_display_container = nullptr;

  SettingsDialog* m_settings_dialog = nullptr;

  QProgressBar* m_status_progress_widget = nullptr;

  QString m_current_disc_path;
  QString m_current_game_serial;
  QString m_current_game_name;
  quint32 m_current_game_crc;
  bool m_emulation_running = false;
  bool m_save_states_invalidated = false;
};

extern MainWindow* g_main_window;
