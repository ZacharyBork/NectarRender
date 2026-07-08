from nectar_render import (
    RenderEngine, Camera, Vector3, Color, Material, Texture, 
    Hittable, Scene, SkyLight, TVDenoiser
)

import sys
import threading
from typing  import Self
from pathlib import Path

import numpy as np

from PySide6 import QtWidgets
from PySide6.QtCore    import QFile, QObject, Signal, Slot
from PySide6.QtUiTools import QUiLoader
from PySide6.QtGui import QShortcut, QKeySequence, QImage, QPixmap

from nectar_render.scenes.cornell_box import CornellBox


class RenderBridge(QObject):
    frame_ready = Signal(int)


class Interface(QObject):    
    def __init__(self: Self) -> None:
        super().__init__()
        
        self.bridge = RenderBridge()
        self.bridge.frame_ready.connect(self._on_frame_finished)

        self.num_samples = 256
        
        self.engine = RenderEngine(
            camera    = Camera(
                resolution   = (512, 512),
                position     = (0.0, 0.0, 2.0),
                rotation     = (0.0, 0.0, 0.0),
                num_samples  = self.num_samples,
                focal_length = 3.0
            ),
            max_depth = 6,
            seed      = None,
            silent    = True
        )
        self.engine.ENGINE.on_frame_finished = (
            self._on_frame_ready_worker_thread
        )
        
        self.engine.ENGINE.on_render_finished = self._on_render_finished
        self.engine.ENGINE.on_canceled = self._on_canceled
        
        self.scene = CornellBox.SCENE
        
        self.reset_render_thread()

    def reset_render_thread(self: Self) -> None:
        self.render_thread = threading.Thread(
            target = self.engine.render, 
            args   = (self.scene,), 
            daemon = True
        )

    ### ENGINE HOOKS ###
    
    def _on_frame_ready_worker_thread(self: Self, frame_idx: int) -> None:
        self.bridge.frame_ready.emit(frame_idx)

    @Slot(int)
    def _on_frame_finished(self: Self, frame_idx: int) -> None:
        data = np.ascontiguousarray(self.engine.get_data())
        H, W, _ = data.shape
        
        qimg = QImage(data.data, W, H, data.strides[0], QImage.Format_RGB888)
        qimg = qimg.copy()
        pixmap = QPixmap.fromImage(qimg)
        self.find(QtWidgets.QLabel, 'image_label').setPixmap(pixmap)
        self.find(QtWidgets.QProgressBar, 'progress_bar').setValue(
            int(frame_idx / self.num_samples * 100.0)
        )
        
    def _on_render_finished(self: Self) -> None:
        self.engine.reset()
        self.reset_render_thread()
        
    def _on_canceled(self: Self) -> None:
        self.engine.reset()
        
    ### ENGINE UTILITIES ###

    def render(self: Self) -> None:
        if self.render_thread.is_alive():
            self.stop_render()
            
        self.find(QtWidgets.QProgressBar, 'progress_bar').setValue(0)
        self.render_thread.start()

    def stop_render(self: Self) -> None:
        self.engine.request_cancel()
        self.render_thread.join()
        self.reset_render_thread()

    ### CAMERA UTILITIES ###
    
    def _update_camera(self: Self) -> None:
        pass


    ### INITIALIZATION ###

    def _init_mainwidget(self: Self) -> QtWidgets.QWidget:
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

    def _init_callbacks(self: Self) -> None:
        self.find(QtWidgets.QPushButton, 'render').clicked.connect(self.render)
        self.find(QtWidgets.QPushButton, 'stop').clicked.connect(self.stop_render)

    def _init_shortcuts(self: Self) -> None:
        key_W = QShortcut('W', self.mainwidget)
        key_W.activated.connect(lambda : print('w pressed'))
        
        key_A = QShortcut('A', self.mainwidget)
        key_A.activated.connect(lambda : print('a pressed'))
        
        key_S = QShortcut('S', self.mainwidget)
        key_S.activated.connect(lambda : print('s pressed'))
        
        key_D = QShortcut('D', self.mainwidget)
        key_D.activated.connect(lambda : print('d pressed'))

    ### ENTRYPOINT ###

    def run(self: Self) -> None:
        self.app = QtWidgets.QApplication(sys.argv)
        self._set_stylesheet()

        self.mainwidget = self._init_mainwidget()
        self.mainwidget.setWindowTitle('NectarRender')
        self.find = self.mainwidget.findChild
        self._init_callbacks()
        self._init_shortcuts()

        self.mainwidget.show()
        sys.exit(self.app.exec())



