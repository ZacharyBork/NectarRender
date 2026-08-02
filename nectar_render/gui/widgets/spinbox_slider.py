from typing  import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, Signal

class SpinboxSlider(W.QWidget):
    updated = Signal(float)
    
    def __init__(
        self:    Self, 
        minimum: float = 0.0,
        maximum: float = 100.0,
        value:   float = 1.0,
        parent:  W.QWidget | None = None
    ) -> None:
        super().__init__(parent=parent)
        layout = W.QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(3)
        self.setLayout(layout)
        
        self.minimum = minimum
        self.maximum = maximum
        self.value   = value
        
        self.slider = W.QSlider(Qt.Orientation.Horizontal)
        self.slider.setMinimum(int(self.minimum * 100.0))
        self.slider.setMaximum(int(self.maximum * 100.0))
        self.slider.setValue(int(self.value * 100.0))
        self.slider.valueChanged.connect(self._slider)
        self.layout().addWidget(self.slider)
        
        self.spinbox = W.QDoubleSpinBox()
        self.spinbox.setMinimum(self.minimum)
        self.spinbox.setMaximum(self.maximum)
        self.spinbox.setValue(self.value)
        self.spinbox.editingFinished.connect(self._spinbox)
        self.layout().addWidget(self.spinbox)

    def get_value(self: Self) -> float:
        return self.spinbox.value()

    def set_value(self: Self, v: float) -> None:
        self.value = v
        self.spinbox.setValue(v)
        self.slider.setValue(int(v * 100.0))
        self.updated.emit(self.value)

    def _spinbox(self: Self) -> None:
        self.value = self.spinbox.value()
        self.slider.setValue(int(self.value * 100.0))
        self.updated.emit(self.value)
        
    def _slider(self: Self) -> None:
        self.value = float(self.slider.value()) / 100.0
        self.spinbox.setValue(self.value)
        self.updated.emit(self.value)
    
