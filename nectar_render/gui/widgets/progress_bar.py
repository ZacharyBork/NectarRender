from typing import Self

import threading
import numpy as np

from PySide6        import QtWidgets
from PySide6.QtCore import Qt, QObject, Signal, Slot
from PySide6.QtGui  import QKeyEvent, QImage, QPixmap

from nectar_render import RenderEngine, Scene, Vector3
from nectar_render.gui.bridge import RenderBridge

###############################################################################
# PROGRESS BAR WIDGET
###############################################################################

class ProgressBar(QtWidgets.QWidget):
    
    def __init__(
        self:   Self, 
        layout: QtWidgets.QLayout
    ) -> None:
        super().__init__()
        self.progress_bar = QtWidgets.QProgressBar()
        self.sample_counter = QtWidgets.QLabel('0/0 samples')
        
        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(3, 3, 3, 3)
        hbox.setSpacing(3)
        
        hbox.addWidget(self.progress_bar)
        hbox.addWidget(self.sample_counter)
        
        self.setLayout(hbox)
        layout.addWidget(self)

    def update(self: Self, current: int, total: int) -> None:
        self.progress_bar.setValue(int(current / max(1, total) * 100.0))
        self.sample_counter.setText(f'{current}/{total} samples')
        
    def reset(self: Self) -> None:
        self.progress_bar.setValue(0)
        self.sample_counter = QtWidgets.QLabel('0/0 samples')
        
        
