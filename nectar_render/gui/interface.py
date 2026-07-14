import sys
from typing  import Self
from pathlib import Path

from PySide6 import QtWidgets as W
from PySide6.QtCore    import QFile, QObject, Slot, QSize, QTimer
from PySide6.QtGui     import QIcon
from PySide6.QtUiTools import QUiLoader

from nectar_render import Camera
from nectar_render.gui.bridge  import RenderBridge, Bridge
from nectar_render.gui.widgets import ViewportWidget, ProgressBar, Profiler
from nectar_render.scenes.cornell_box import CornellBox

###############################################################################
# INTERFACE CLASS
###############################################################################

class Interface(QObject):    
    def __init__(self: Self) -> None:
        super().__init__()
        
        self.viewport:  ViewportWidget = None
        self.progress_bar: ProgressBar = None
        self.profiler:        Profiler = None
        
        self.scene = CornellBox.SCENE
        self.max_depth: int = 6
        self.seed: int | None = None

        bridge = RenderBridge(
            self.scene, 
            Camera(
                resolution   = (512, 512),
                position     = (0.0, 0.0, 2.0),
                rotation     = (0.0, 0.0, 0.0),
                num_samples  = 2048,
                focal_length = 3.0
            ), 
            self.max_depth, 
            self.seed
        )
        bridge.signals.paused.connect(self._on_paused)
        bridge.signals.canceled.connect(self._on_canceled)
        bridge.signals.frame_finished.connect(self._on_frame_finished)
        bridge.signals.render_finished.connect(self._on_render_finished)
        Bridge.set_instance(bridge)

        self.update_timer = QTimer(self)
        self.update_timer.setInterval(16)
        self.update_timer.timeout.connect(self._poll_updates)
        
#### ENGINE UTILITIES #########################################################

    def _poll_updates(self: Self) -> None:
        self.viewport.update_camera()
        
#### ENGINE HOOKS #############################################################
    
    @Slot(int)
    def _on_frame_finished(self: Self, frame_idx: int) -> None:
        self.viewport.update_image()
        self.progress_bar.update(frame_idx, Bridge.instance.n_samples)
        
    def _on_render_finished(self: Self) -> None:
        Bridge.instance.reset()
        
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
      
    def _on_paused  (self: Self) -> None: pass
    def _on_canceled(self: Self) -> None: Bridge.instance.reset()
        
#### CALLBACKS ################################################################

    def toggle_groupbox_widgets(self: Self, name: str) -> None:
        box = self.find(W.QGroupBox, name)
        widgets = [i for i in box.children() if hasattr(i, 'setVisible')]
        
        for widget in widgets:
            visible = not widget.isVisible()
            widget.setVisible(visible)
            if visible: box.setStyleSheet('padding: 3px;')
            else: box.setStyleSheet('padding: -5px;')

    def play_button(self: Self) -> None:
        Bridge.instance.start()
        self.find(W.QPushButton, 'play').setEnabled(False)
        self.find(W.QPushButton, 'pause').setEnabled(True)
        self.find(W.QPushButton, 'stop').setEnabled(True)
        
    def pause_button(self: Self) -> None:
        Bridge.instance.pause()
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(True)
        
    def stop_button(self: Self) -> None:
        Bridge.instance.stop()
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
        
    def refresh(self: Self) -> None:
        Bridge.instance.request_reset()

    def set_n_samples(self: Self) -> None:
        if Bridge.instance.ENGINE.is_rendering():
            Bridge.instance.request_pause()
        else: self._run_camera_update()
        
    def set_n_samples(self: Self) -> None:
        if Bridge.instance.ENGINE.is_rendering():
            Bridge.instance.signals.paused.connect(self._set_n_samples)
            Bridge.instance.request_pause()
        else: self._set_n_samples()
    
    @Slot()
    def _set_n_samples(self: Self) -> None:
        Bridge.instance.signals.paused.disconnect(self._set_n_samples)
        with Bridge.reset(): 
            Bridge.instance.ENGINE.set_n_samples(
                self.find(W.QSpinBox, 'n_samples').value()
            )
            
    def set_max_depth(self: Self) -> None:
        if Bridge.instance.ENGINE.is_rendering():
            Bridge.instance.signals.paused.connect(self._set_max_depth)
            Bridge.instance.request_pause()
        else: self._set_max_depth()
    
    @Slot()
    def _set_max_depth(self: Self) -> None:
        Bridge.instance.signals.paused.disconnect(self._set_max_depth)
        with Bridge.reset(): 
            Bridge.instance.ENGINE.set_max_depth(
                self.find(W.QSpinBox, 'max_depth').value()
            )
            
    def _select_render_pass(self: Self, index: int) -> None:
        # TODO: Implement render pass switching
        pass

#### INITIALIZATION ###########################################################

    def _init_mainwidget(self: Self) -> W.QWidget:
        path = Path(__file__).parent.resolve() / 'resource/mainwidget.ui'
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
        self.viewport = ViewportWidget(
            self.find(W.QGroupBox, 'camera_settings')
        )
        frame = self.find(W.QFrame, 'viewport_frame')
        frame.layout().addWidget(self.viewport)

    def _init_callbacks(self: Self) -> None:        
        connect_button = lambda name, fn : (
            self.find(W.QPushButton, name).clicked.connect(fn)
        )
        
        connect_button('play',    self.play_button)
        connect_button('pause',   self.pause_button)
        connect_button('stop',    self.stop_button)
        connect_button('refresh', self.refresh)
        connect_button('save_render', self.viewport.save_image)
        
        connect_spinbox = lambda name, fn : (
            self.find(W.QSpinBox, name).editingFinished.connect(fn)
        )
        
        connect_spinbox('n_samples', self.set_n_samples)
        connect_spinbox('max_depth', self.set_max_depth)
        
        connect_groupbox = lambda name : (
            self.find(W.QGroupBox, name).clicked.connect(
                lambda : self.toggle_groupbox_widgets(name)
            )
        )

        connect_groupbox('general_settings')
        connect_groupbox('camera_settings')
        
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
        
        render_pass = self.find(W.QComboBox, 'render_pass')
        render_pass.currentIndexChanged.connect(self._select_render_pass)
        render_pass.addItem('Beauty')
        render_pass.addItem('Normal (WS)')
        
    def _build_profiler(self: Self) -> None:
        tab = self.find(W.QWidget, 'profiler_tab')
        self.profiler = Profiler(tab)        

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
        self._build_profiler()
        self._init_callbacks()
        self.progress_bar = ProgressBar(
            self.find(W.QFrame, 'progress_frame').layout()
        )
        
        self.update_timer.start()
        self.mainwidget.show()
        
        sys.exit(self.app.exec())



