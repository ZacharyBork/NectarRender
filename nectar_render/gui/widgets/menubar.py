from __future__ import annotations
from typing     import TYPE_CHECKING
if TYPE_CHECKING:
    from nectar_render.gui.interface import Interface

import sys
from typing  import Self
from collections.abc import Callable

from PySide6 import QtWidgets as W
from PySide6.QtCore import Slot
from PySide6.QtGui import QAction

from nectar_render.gui.bridge  import Bridge
from nectar_render.python.cuda import cudaDeviceSynchronize

class MenuBar:
    def __init__(self: Self, interface: Interface) -> None:
        self.interface = interface
        self.bar = self.interface.mainwidget.findChild(W.QMenuBar)
        self.bar.setNativeMenuBar(False)
        
        # FILE MENU
        
        file_menu = self.bar.addMenu('&File')
        self.add_action(
            file_menu, 'Quit', self._quit_application, 
            'Quit application', 'Ctrl+Q'
        )

        # SCENE MENU
        
        scene_menu = self.bar.addMenu('&Scene')
        self.add_action(
            scene_menu, 'Reload', self.reload_scene, 'Reload current scene'
        )

#### UTILS ####################################################################

    def add_action(
        self:       Self,
        menu:       W.QMenu,
        title:      str,
        callback:   Callable [[], None],
        status_tip: str | None = None,
        shortcut:   str | None = None
    ) -> None:
        action = QAction(title, self.interface.mainwidget)
        action.triggered.connect(callback)
        if shortcut is not None: action.setShortcut(shortcut)
        if status_tip is not None: action.setStatusTip(status_tip)
        menu.addAction(action)

#### QUIT #####################################################################

    def _quit_application(self: Self) -> None:
        reply = W.QMessageBox.question(
            self.interface.mainwidget, 'Confirm',
            'Are you sure you want to quit? All unsaved changes will be lost.',
            W.QMessageBox.Yes | W.QMessageBox.No, W.QMessageBox.No
        )

        if reply == W.QMessageBox.Yes:
            Bridge.signals.shutdown.connect(self._on_quit)
            Bridge.request_shutdown()
        
    @Slot()
    def _on_quit(self: Self) -> None:
        cudaDeviceSynchronize();
        Bridge.thread.stop()
        sys.exit()
        
#### RELOAD SCENE #############################################################
            
    def reload_scene(self: Self) -> None:
        Bridge.signals.shutdown.connect(self._reload_scene)
        Bridge.request_shutdown()
            
    @Slot()
    def _reload_scene(self: Self) -> None:
        cudaDeviceSynchronize();
        Bridge.thread.stop()
        Bridge.signals.shutdown.disconnect(self._reload_scene)
        Bridge.set_scene(self.interface.scene)
        Bridge.thread.start()


