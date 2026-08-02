from typing import Self
from PySide6 import QtWidgets as W
from PySide6.QtCore import Slot, QPointF

from nectar_render import SceneInterface, Transform, Vector2, Vector3, Color
from nectar_render.gui.bridge import Bridge
from nectar_render.gui import utils
   
   
class VectorWidget(W.QWidget):
    def __init__(
        self:      Self,
        values:    tuple[float, float, float] = (0.0, 0.0, 0.0),
        min_value: float = -999999.0,
        max_value: float =  999999.0
    ) -> None:
        super().__init__()
        self.values = values
        self._locked = False
        
        layout = W.QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        self.setStyleSheet(
            'padding: 2px, 2px; '
            'min-width: 18px; '
        )
   
        self._axes: dict[str, W.QDoubleSpinBox] = {}
        for idx, axis in enumerate(['x', 'y', 'z']):
            widget = W.QDoubleSpinBox()
            widget.setDecimals(2)
            widget.setMinimum(min_value)
            widget.setMaximum(max_value)
            widget.setValue(self.values[idx])
            widget.setButtonSymbols(
                W.QAbstractSpinBox.ButtonSymbols.NoButtons
            )
            
            
            
            self._axes[axis] = widget
            layout.addWidget(widget)

        self._axes['x'].setStyleSheet('color: rgb(255, 100, 100);')
        self._axes['y'].setStyleSheet('color: rgb(100, 255, 100);')
        self._axes['z'].setStyleSheet('color: rgb(100, 100, 255);')

        self.reset = W.QPushButton()
        
        self.reset.setSizePolicy(
            W.QSizePolicy.Policy.Maximum, W.QSizePolicy.Policy.Preferred
        )
        utils.set_button_icon(self.reset, 'reset', (16, 16))
        self.reset.clicked.connect(self._reset)
        layout.addWidget(self.reset)
        
        self.lock = W.QPushButton()
        self.lock.setSizePolicy(
            W.QSizePolicy.Policy.Maximum, W.QSizePolicy.Policy.Preferred
        )
        utils.set_button_icon(self.lock, 'lock_open', (16, 16))
        self.lock.clicked.connect(self._toggle_lock)
        layout.addWidget(self.lock)

        self.setLayout(layout)
        
    @property
    def x(self: Self) -> float: return self._axes['x'].value()
    @x.setter
    def x(self: Self, value: float) -> None: self._axes['x'].setValue(value)
    @property
    def y(self: Self) -> float: return self._axes['y'].value()
    @y.setter
    def y(self: Self, value: float) -> None: self._axes['y'].setValue(value)
    @property
    def z(self: Self) -> float: return self._axes['z'].value()
    @z.setter
    def z(self: Self, value: float) -> None: self._axes['z'].setValue(value)
    @property
    def is_locked(self: Self) -> bool: return self._locked
    
    def _reset(self: Self) -> None:
        self.x, self.y, self.z = self.values
        
    def _toggle_lock(self: Self) -> None:
        self._locked = not self._locked
        for widget in list(self._axes.values()) + [self.reset]:
            widget.setDisabled(self._locked)
        icon = 'lock' if self._locked else 'lock_open'
        self.lock.setStyleSheet(
            'background-color: #ff5e3a;' if self._locked else
            'background-color: #1f2128;'
        )
        utils.set_button_icon(self.lock, icon, (16, 16))
        
    def as_vector3(self: Self) -> Vector3:
        return Vector3(self.x, self.y, self.z)
    
    def as_color(self: Self) -> Color:
        return Color(self.x, self.y, self.z)
    
    def set_from_vector(self: Self, v: Vector3) -> None:
        self.x = v.x(); self.y = v.y(); self.z = v.z()
        
    def set_from_color(self: Self, c: Color) -> None:
        self.x = c.r(); self.y = c.g(); self.z = c.b()
