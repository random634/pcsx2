set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(pcsx2-qt)

if (PACKAGE_MODE)
	install(TARGETS pcsx2-qt DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
	install(TARGETS pcsx2-qt DESTINATION ${CMAKE_SOURCE_DIR}/bin)
endif()

target_sources(pcsx2-qt PRIVATE
	BIOSSettingsWidget.cpp
	BIOSSettingsWidget.h
	BIOSSettingsWidget.ui
	DisplaySettingsWidget.cpp
	DisplaySettingsWidget.h
	DisplaySettingsWidget.ui
	DisplayWidget.cpp
	DisplayWidget.h
	EmuThread.cpp
	EmuThread.h
	EmulationSettingsWidget.cpp
	EmulationSettingsWidget.h
	EmulationSettingsWidget.ui
	GameListModel.cpp
	GameListModel.h
	GameListRefreshThread.cpp
	GameListRefreshThread.h
	GameListSettingsWidget.cpp
	GameListSettingsWidget.h
	GameListSettingsWidget.ui
	GameListWidget.cpp
	GameListWidget.h
	GraphicsSettingsWidget.cpp
	GraphicsSettingsWidget.h
	GraphicsSettingsWidget.ui
	InterfaceSettingsWidget.cpp
	InterfaceSettingsWidget.h
	InterfaceSettingsWidget.ui
	Main.cpp
	MainWindow.cpp
	MainWindow.h
	MainWindow.ui
	PrecompiledHeader.cpp
	PrecompiledHeader.h
	QtKeyCodes.cpp
	QtHost.cpp
	QtHost.h
	QtUtils.cpp
	QtUtils.h
	SettingsDialog.cpp
	SettingsDialog.h
	SettingsDialog.ui
	SettingWidgetBinder.h
	SystemSettingsWidget.cpp
	SystemSettingsWidget.h
	SystemSettingsWidget.ui
	resources/resources.qrc
)

target_precompile_headers(pcsx2-qt PRIVATE PrecompiledHeader.h)

target_include_directories(pcsx2-qt PRIVATE
	${Qt5Gui_PRIVATE_INCLUDE_DIRS}
	"${CMAKE_BINARY_DIR}/common/include"
	"${CMAKE_SOURCE_DIR}/pcsx2"
)

target_link_libraries(pcsx2-qt PRIVATE
	PCSX2_FLAGS
	PCSX2
	common
	yaml-cpp
	simpleini
	wx30_base
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::Network
)
