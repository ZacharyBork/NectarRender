from typing  import Self
from PySide6 import QtWidgets as W

from nectar_render.gui.widgets.collapsible_menu import CollapsibleMenu

class DoublingSpinBox(W.QSpinBox):
    def stepBy(self, steps):
        if steps > 0:
            self.setValue(self.value() * 2)
        elif steps < 0:
            self.setValue(max(self.minimum(), self.value() // 2))

class EngineSettings(CollapsibleMenu):
    def __init__(self: Self) -> None:
        super().__init__(
            name='Engine', start_closed=True, parent=None
        )

        settings_frame = W.QFrame()
        settings_frame.setStyleSheet('padding: 2px;')
        
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        settings_frame.setLayout(layout)

        self.res_x = W.QSpinBox()
        self.res_x.setRange(1, 8192)
        self.res_x.setValue(512)
        self.res_x.setButtonSymbols(W.QAbstractSpinBox.NoButtons)
        self.res_x.editingFinished.connect(self.set_resolution)
        
        self.res_y = W.QSpinBox()
        self.res_y.setRange(1, 8192)
        self.res_y.setValue(512)
        self.res_y.setButtonSymbols(W.QAbstractSpinBox.NoButtons)
        self.res_y.editingFinished.connect(self.set_resolution)

        l = W.QHBoxLayout()
        l.addWidget(self.res_x)
        l.addWidget(self.res_y)
        layout.addRow('Resolution', l)

        self.n_samples = DoublingSpinBox()
        self.n_samples.setRange(1, 32768)
        self.n_samples.setValue(1024)
        self.n_samples.setSingleStep(256)
        self.n_samples.editingFinished.connect(self.set_n_samples)
        layout.addRow('Samples', self.n_samples)

        self.max_depth = W.QSpinBox()
        self.max_depth.setRange(1, 64)
        self.max_depth.setValue(6)
        self.max_depth.editingFinished.connect(self.set_max_depth)
        layout.addRow('Max Depth', self.max_depth)

        self.add_widget(settings_frame)


    def set_resolution(self: Self) -> None:
        raise NotImplementedError
    

    def set_n_samples(self: Self) -> None:
        raise NotImplementedError


    def set_max_depth(self: Self) -> None:
        raise NotImplementedError
    
