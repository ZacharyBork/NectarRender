from __future__ import annotations
from typing     import Self, TYPE_CHECKING
if TYPE_CHECKING:
    from nectar_render.gui.widgets import ViewportWidget

import numpy as np

from PySide6 import QtWidgets as W
from PySide6.QtGui import QPainter, QPen, QColor, QFont
from PySide6.QtCore import QPointF, QEvent

from nectar_render.python.core import Matrix3
from nectar_render.gui.bridge import Bridge

class GnomonWidget(W.QWidget):
    def __init__(
        self:     Self, 
        viewport: ViewportWidget,
        size:     int = 90
    ) -> None:
        super().__init__(viewport)
        self.setFixedSize(size, size)
        self.rot = np.array([
            [1.0, 0.0, 0.0], 
            [0.0, 1.0, 0.0], 
            [0.0, 0.0, 1.0]
        ], dtype=np.float32)

    def update_rotation(self: Self) -> None:
        self.rot = Bridge.camera.parameters().R.numpy()
        self.rot = np.array(self.rot)
        
        self.update()

    def paintEvent(self: Self, event: QEvent) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        cx, cy = self.width() / 2, self.height() / 2
        length = min(cx, cy) * 0.8

        axes = [((1,0,0), QColor(220,60,60), 'X'),
                ((0,1,0), QColor(60,200,80), 'Y'),
                ((0,0,1), QColor(70,120,230), 'Z')]

        projected = []
        for vec, color, label in axes:
            rv = self._apply(self.rot, vec)
            projected.append((rv[2], rv, color, label))
        projected.sort(key=lambda t: t[0])

        for _, rv, color, label in projected:
            end = QPointF(cx + rv[0]*length, cy - rv[1]*length)
            p.setPen(QPen(color, 3))
            p.drawLine(QPointF(cx, cy), end)
            p.setFont(QFont('Arial', 9, QFont.Bold))
            p.drawText(end, label)

    @staticmethod
    def _apply(m: np.ndarray, v):
        return tuple(sum(m[i][j] * v[j] for j in range(3)) for i in range(3))
    
    
