import sys
import threading
from typing  import Self
from pathlib import Path

from PySide6 import QtWidgets as W
from PySide6.QtCore    import Qt, QFile, QObject, Signal, Slot, QSize
from PySide6.QtGui     import QKeyEvent, QIcon, QPixmap
from PySide6.QtUiTools import QUiLoader

from nectar_render import RenderEngine, Camera, Vector3
from nectar_render.gui.bridge  import RenderBridge
from nectar_render.gui.widgets import ViewportWidget, ProgressBar
from nectar_render.scenes.cornell_box import CornellBox

###############################################################################
# INTERFACE CLASS
###############################################################################

class Interface(QObject):    
    def __init__(self: Self) -> None:
        super().__init__()
        
        self.viewport:  ViewportWidget = None
        self.progress_bar: ProgressBar = None
        
        self.scene  = CornellBox.SCENE
        self.camera = Camera(
            resolution   = (512, 512),
            position     = (0.0, 0.0, 2.0),
            rotation     = (0.0, 0.0, 0.0),
            num_samples  = 256,
            focal_length = 3.0
        )

        self.max_depth:   int = 6
        self.seed: int | None = None

        self.bridge = RenderBridge(
            self.scene, self.camera, self.max_depth, self.seed
        )
        self.bridge.signals.paused.connect(self._on_paused)
        self.bridge.signals.canceled.connect(self._on_canceled)
        self.bridge.signals.frame_finished.connect(self._on_frame_finished)
        self.bridge.signals.render_finished.connect(self._on_render_finished)

#### ENGINE UTILITIES #########################################################

    def update_engine(self: Self) -> None:
        self.viewport.update_camera()
        
#### ENGINE HOOKS #############################################################
    
    @Slot(int)
    def _on_frame_finished(self: Self, frame_idx: int) -> None:
        self.viewport.update_image()
        self.progress_bar.update(frame_idx, self.camera.n_samples)
        self.update_engine()
        
    def _on_render_finished(self: Self) -> None:
        self.bridge.reset()
        
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
      
    def _on_paused  (self: Self) -> None: pass
    def _on_canceled(self: Self) -> None: self.bridge.reset()
        
#### CALLBACKS ################################################################

    def play_button(self: Self) -> None:
        self.bridge.start()
        self.find(W.QPushButton, 'play').setEnabled(False)
        self.find(W.QPushButton, 'pause').setEnabled(True)
        self.find(W.QPushButton, 'stop').setEnabled(True)
        
    def pause_button(self: Self) -> None:
        self.bridge.pause()
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(True)
        
    def stop_button(self: Self) -> None:
        self.bridge.stop()
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
        
    def refresh(self: Self) -> None:
        self.bridge.request_refresh()
        
#### INITIALIZATION ###########################################################

    def _init_mainwidget(self: Self) -> W.QWidget:
        path = Path(__file__).parent.resolve() / 'resource/mainwidget2.ui'
        if not path.exists():
            msg = f'Unable to locate UI file: {path.resolve().as_posix()}'
            raise FileNotFoundError(msg)
        
        file = QFile(path.as_posix())
        file.open(QFile.ReadOnly)
        widget = QUiLoader().load(file)
        file.close()
        return widget

    def _set_stylesheet(self: Self) -> None:
        path = Path(__file__).parent.resolve() / 'resource/stylesheet.qss'
        if not path.exists():
            raise FileNotFoundError(
                f'Unable to locate stylesheet: {path.as_posix()}'
            )
        with open(path.as_posix(), 'r') as file:
            self.app.setStyleSheet(file.read())

    def _build_viewport(self: Self) -> None:
        self.viewport = ViewportWidget(self.bridge)
        frame = self.find(W.QFrame, 'viewport_frame')
        frame.layout().addWidget(self.viewport)

    def _init_callbacks(self: Self) -> None:        
        connect_button = lambda name, fn : (
            self.find(W.QPushButton, name).clicked.connect(fn)
        )
        
        connect_button('play',     self.play_button)
        connect_button('pause',    self.pause_button)
        connect_button('stop',     self.stop_button)
        connect_button('refresh',  self.refresh)
        
    def _build_control_bar(self: Self) -> None:
        root = Path(__file__).parent.resolve() / 'resource/icons'
        
        icon_paths = {
            'play':        ('play.png',    (16, 16)),
            'pause':       ('pause.png',   (16, 16)),
            'stop':        ('stop.png',    (16, 16)),
            'refresh':     ('refresh.png', (16, 16)),
            'save_render': ('save.png',    (16, 16))
        }
        
        for name, (file, size) in icon_paths.items():
            path = root / file
            btn = self.find(W.QPushButton, name)
            btn.setIcon(QIcon(path.as_posix()))
            btn.setIconSize(QSize(*size))
            
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)

#### ENTRYPOINT ###############################################################

    def run(self: Self) -> None:
        self.app = W.QApplication(sys.argv)
        self._set_stylesheet()

        self.mainwidget = self._init_mainwidget()
        self.mainwidget.setWindowTitle('NectarRender')
        self.mainwidget.menuBar().setNativeMenuBar(False)
        self.find = self.mainwidget.findChild
        
        self._build_viewport()
        self._build_control_bar()
        self._init_callbacks()
        self.progress_bar = ProgressBar(
            self.find(W.QFrame, 'progress_frame').layout()
        )
        
        self.mainwidget.show()
        sys.exit(self.app.exec())



