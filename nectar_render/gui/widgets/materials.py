from typing import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, QObject, Slot, Signal

from nectar_render import SceneInterface, Color, Material
from nectar_render.gui.bridge import Bridge
from nectar_render.gui.widgets.xform_controller import VectorWidget

###############################################################################
# ABSTRACT PARENT
###############################################################################

class MatSettings(W.QFrame):
    def __init__(self: Self) -> None:
        super().__init__()
        # self.setStyleSheet(
        #     'padding: 2px, 2px; '
        #     'min-height: 18px; '
        #     'max-height: 18px; '
        #     'min-width: 18px; '
        # )

    def get_material(self: Self) -> Material:        
        raise NotImplementedError

###############################################################################
# PBR METAL ROUGHNESS
###############################################################################

class MatSettingsPBR(MatSettings):
    def __init__(self: Self) -> None:
        super().__init__()
        
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        self.albedo = VectorWidget(
            (1.0, 1.0, 1.0), min_value=0.0, max_value=1.0
        )
        layout.addRow('Albedo', self.albedo)
        
        self.roughness = W.QSlider(Qt.Orientation.Horizontal)
        self.roughness.setMinimum(0); self.roughness.setMaximum(100)
        layout.addRow('Roughness', self.roughness)
        
        self.metallic = W.QSlider(Qt.Orientation.Horizontal)
        self.metallic.setMinimum(0); self.metallic.setMaximum(100)
        layout.addRow('Metallic', self.metallic)
        
        self.emission = VectorWidget(
            (0.0, 0.0, 0.0), min_value=0.0, max_value=1.0
        )
        layout.addRow('Emission', self.emission)
        self.setLayout(layout)

    def get_material(self: Self) -> Material.PBR:        
        return Material.PBR(
            self.albedo.as_color(),
            self.roughness.value(),
            self.metallic.value(),
            self.emission.value()
        )

###############################################################################
# DIELECTRIC
###############################################################################

class MatSettingsDielectric(MatSettings):
    def __init__(self: Self) -> None:
        super().__init__()

    def get_material(self: Self) -> Material:        
        raise NotImplementedError

###############################################################################
# ISOTROPIC
###############################################################################

class MatSettingsIsotropic(MatSettings):
    def __init__(self: Self) -> None:
        super().__init__()

    def get_material(self: Self) -> Material:        
        raise NotImplementedError



