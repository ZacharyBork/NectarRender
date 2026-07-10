from typing import Self

import threading
import numpy as np
from dataclasses import dataclass

from PySide6        import QtWidgets
from PySide6.QtCore import Qt, QObject, Signal, Slot
from PySide6.QtGui  import QKeyEvent, QImage, QPixmap

from nectar_render import RenderEngine, Scene, Vector3
from nectar_render.gui.bridge import RenderBridge

###############################################################################
# VIEWPORT WIDGET
###############################################################################

@dataclass
class FrameBuffer:
    data: np.ndarray
    
    @property
    def C(self: Self) -> int: return self.data.shape[2]
    @property
    def H(self: Self) -> int: return self.data.shape[0]
    @property
    def W(self: Self) -> int: return self.data.shape[1]
    @property
    def strides(self: Self) -> int: return self.data.strides[0]
    
###############################################################################
# VIEWPORT WIDGET
###############################################################################

class ViewportWidget(QtWidgets.QLabel):
        
    def __init__(self: Self, bridge: RenderBridge) -> None:
        super().__init__()
        
        self.bridge = bridge
        self.buffer: FrameBuffer = None
        
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self._held_keys: set[Qt.Key] = set()
        
        self._build_image_label()
    
#### INITIALIZATION ###########################################################
        
    def _build_image_label(self: Self) -> None:
        self.image_label = QtWidgets.QLabel('__image_label__')
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        self.setLayout(QtWidgets.QVBoxLayout())
        self.layout().addWidget(self.image_label)

#### KEYPRESS UTILITIES #######################################################

    def keyPressEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat(): return
        self._held_keys.add(event.key())

    def keyReleaseEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat():
            return
        self._held_keys.discard(event.key())

    def is_held(self: Self, key: Qt.Key) -> bool:
        return key in self._held_keys

#### CAMERA UTILITIES #########################################################
    
    def update_camera(self: Self) -> None:
        delta_p = [0.0, 0.0, 0.0]
        if self.is_held(Qt.Key.Key_W): delta_p[2] += 1.0
        if self.is_held(Qt.Key.Key_S): delta_p[2] -= 1.0
        if self.is_held(Qt.Key.Key_A): delta_p[0] -= 1.0
        if self.is_held(Qt.Key.Key_D): delta_p[0] += 1.0
        
        # print(delta_p)
        
        # if (abs(sum(delta_p)) > 0.0):
        #     self.bridge.reset()
        
        # if (abs(sum(delta_p)) > 0.0):
        #     self.bridge.camera.update(
        #         Vector3(delta_p[0], delta_p[1], delta_p[2]), 
        #         Vector3(0.0, 0.0, 0.0)
        #     )

#### IMAGE UTILITIES ##########################################################
    
    def update_image(self: Self) -> None:
        self.buffer = FrameBuffer(np.ascontiguousarray(self.bridge.get_data()))
        
        qimg = QImage(
            self.buffer.data, self.buffer.W, self.buffer.H, 
            self.buffer.strides, QImage.Format_RGB888
        )
        qimg = qimg.copy()
        pixmap = QPixmap.fromImage(qimg)
        self.image_label.setPixmap(pixmap)



