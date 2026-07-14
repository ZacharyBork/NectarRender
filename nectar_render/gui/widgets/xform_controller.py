from typing import Self
from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt
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
        self.locked = False
        
        layout = W.QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        self._axes: dict[str, W.QDoubleSpinBox] = {}
        for idx, axis in enumerate(['x', 'y', 'z']):
            widget = W.QDoubleSpinBox()
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
    def y(self: Self) -> float: return self._axes['y']
    @y.setter
    def y(self: Self, value: float) -> None: self._axes['y'].setValue(value)
    @property
    def z(self: Self) -> float: return self._axes['z']
    @z.setter
    def z(self: Self, value: float) -> None: self._axes['z'].setValue(value)
    
    def _reset(self: Self) -> None:
        self.x, self.y, self.z = self.values
        
    def _toggle_lock(self: Self) -> None:
        self.locked = not self.locked
        for widget in list(self._axes.values()) + [self.reset]:
            widget.setDisabled(self.locked)
        icon = 'lock' if self.locked else 'lock_open'
        utils.set_button_icon(self.lock, icon, (16, 16))
        
class XformController(W.QWidget):
    def __init__(self: Self, parent: W.QWidget | None = None) -> None:
        super().__init__(parent=parent)
        self.setObjectName('xform_controller')
        
        layout = W.QVBoxLayout()
        self.setLayout(layout)

        self.translation = VectorWidget()
        self.rotation    = VectorWidget()
        self.scale       = VectorWidget(values=(1.0, 1.0, 1.0))
        
        layout = W.QFormLayout()
        layout.addRow('Translate', self.translation)
        layout.addRow('Rotate',    self.rotation)
        layout.addRow('Scale',     self.scale)
        
        frame = W.QFrame()
        frame.setLayout(layout)
        self.layout().addWidget(frame)
        

