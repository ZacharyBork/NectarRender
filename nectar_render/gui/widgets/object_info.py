from typing import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, Slot

from nectar_render import ObjectInterface, Material, Color
from nectar_render.gui.bridge import RenderBridge, BridgeReset

class MaterialSettings(W.QGroupBox):
    def __init__(
        self:      Self, 
        bridge:    RenderBridge, 
        interface: ObjectInterface
    ) -> None:
        super().__init__()
        self.bridge = bridge
        self.interface = interface
        
        self.setTitle('Material')
        
        self.r_slider: W.QSlider = None
        self.g_slider: W.QSlider = None
        self.b_slider: W.QSlider = None
        
        sliders = self._build_sliders()
        
        layout = W.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(sliders)
        
        self.update_btn = W.QPushButton('Update')
        self.update_btn.clicked.connect(self.update_material)
        layout.addWidget(self.update_btn)
        
        self.setLayout(layout)
        
    def _build_sliders(self: Self) -> W.QFrame:
        sliders: list[W.QSlider] = []
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        for tag in ['R', 'G', 'B']:
            slider = W.QSlider(Qt.Orientation.Horizontal)
            slider.setMinimum(0); slider.setMaximum(255)
            sliders.append(slider)
            layout.addRow(tag, slider)
            
        self.r, self.g, self.b = sliders
        frame = W.QFrame()
        frame.setLayout(layout)
        return frame
        
    def update_material(self: Self) -> None:
        if self.bridge.ENGINE.is_rendering():
            self.bridge.signals.paused.connect(self._update_material)
            self.bridge.request_pause()
        else: self._update_material()
       
    @Slot()
    def _update_material(self: Self) -> None:
        self.bridge.signals.paused.disconnect(self._update_material)
        color = Color(
            self.r.value() / 255.0,
            self.g.value() / 255.0,
            self.b.value() / 255.0
        )
        with BridgeReset():
            self.interface.update_material(Material.LAMBERTIAN(color))


class ObjectInfo(W.QFrame):
    def __init__(
        self:      Self, 
        bridge:    RenderBridge, 
        interface: ObjectInterface, 
        parent:    W.QWidget
    ) -> None:
        super().__init__(parent=parent)
        self.setObjectName('object_info')
        
        self.bridge = bridge
        self.interface = interface
        
        layout = W.QVBoxLayout()
        self.close_btn = W.QPushButton('X')
        layout.addWidget(self.close_btn)
        
        self.mat_settings = MaterialSettings(bridge, interface)
        layout.addWidget(self.mat_settings)
        
        
        self.setLayout(layout)
        
    


