from typing import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, Slot, Signal

from nectar_render import ObjectInterface, Color, Material as M
from nectar_render.gui.bridge import BridgeReset, Bridge

class MaterialSettings(W.QGroupBox):
    def __init__(
        self:      Self, 
        interface: ObjectInterface
    ) -> None:
        super().__init__()
        self.interface = interface
        
        self.setTitle('Material')
        layout = W.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)
        
        self.material_type = W.QComboBox()
        self.material_type.addItem('Lambertian')
        self.material_type.addItem('Metal')
        self.material_type.addItem('Dielectric')
        self.material_type.addItem('Emissive')
        layout.addWidget(self.material_type)
        
        self.r_slider: W.QSlider = None
        self.g_slider: W.QSlider = None
        self.b_slider: W.QSlider = None
        
        sliders = self._build_sliders()
        layout.addWidget(sliders)
        
        self.roughness = W.QSlider(Qt.Orientation.Horizontal)
        self.roughness.setMinimum(0); self.roughness.setMaximum(100)
        
        self.ior = W.QSlider(Qt.Orientation.Horizontal)
        self.ior.setMinimum(0); self.ior.setMaximum(500)
        
        self.brightness = W.QSlider(Qt.Orientation.Horizontal)
        self.brightness.setMinimum(0); self.brightness.setMaximum(500)
        
        f = W.QFrame()
        l = W.QFormLayout()
        f.setLayout(l)
        l.addRow('Roughness', self.roughness)
        l.addRow('IOR', self.ior)
        l.addRow('Brightness', self.brightness)
        layout.addWidget(f)
        
        self.update_btn = W.QPushButton('Update')
        self.update_btn.clicked.connect(self.update_material)
        layout.addWidget(self.update_btn)
        
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
        bridge = Bridge.acquire()
        if bridge.ENGINE.is_rendering():
            bridge.signals.paused.connect(self._update_material)
            bridge.request_pause()
        else: self._update_material()
       
    @Slot()
    def _update_material(self: Self) -> None:
        Bridge.acquire().signals.paused.disconnect(self._update_material)
        color = Color(
            self.r.value() / 255.0,
            self.g.value() / 255.0,
            self.b.value() / 255.0
        )
        match self.material_type.currentIndex():
            case 0: m = M.LAMBERTIAN(color)
            case 1: m = M.METAL(color, self.roughness.value() * 0.01)
            case 2: m = M.DIELECTRIC(self.ior.value() * 0.01)
            case 3: m = M.EMISSIVE(color, self.brightness.value() * 0.01)
    
        with BridgeReset():
            self.interface.update_material(m)


class ObjectInfo(W.QFrame):
    close_signal = Signal()

    def __init__(
        self:      Self, 
        interface: ObjectInterface, 
        parent:    W.QWidget
    ) -> None:
        super().__init__(parent=parent)
        self.setObjectName('object_info')
        
        self.interface = interface
        
        layout = W.QVBoxLayout()
        self.close_btn = W.QPushButton('X')
        self.close_btn.clicked.connect(lambda : self.close_signal.emit())
        layout.addWidget(self.close_btn)
        
        self.mat_settings = MaterialSettings(interface)
        layout.addWidget(self.mat_settings)
        
        self.setLayout(layout)
        
    


