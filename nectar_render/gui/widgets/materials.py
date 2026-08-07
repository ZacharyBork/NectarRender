from typing import Self

from PySide6 import QtWidgets as W

from nectar_render import Color, Material, Texture, TextureType
from nectar_render.gui import utils
from nectar_render.gui.widgets.spinbox_slider import SpinboxSlider
from nectar_render.gui.widgets.file_selector import FileSelector
from nectar_render.gui.widgets.vector import VectorWidget

###############################################################################
# TEXTURE WIDGETS
###############################################################################

class ColorTextureWidget(W.QWidget):
    def __init__(
        self:    Self,
        texture: Texture
    ) -> None:
        super().__init__()
        self.setLayout(W.QHBoxLayout())
        self.layout().setContentsMargins(0, 0, 0, 0)
        
        self.constant_color = VectorWidget()
        self.file_selector  = FileSelector()
        
        self.layout().addWidget(self.constant_color)
        self.layout().addWidget(self.file_selector)
        
        self.swap_type_btn = W.QPushButton()
        self.swap_type_btn.setStyleSheet('padding: 3px, 3px;')
        self.swap_type_btn.clicked.connect(
            lambda : self._set_interface_type(
                TextureType.CONSTANT 
                if self.texture_type == TextureType.IMAGE
                else TextureType.IMAGE
            )
        )
        self.layout().addWidget(self.swap_type_btn)
        
        self.texture = texture
        self.texture_type = self.texture.type
        self._set_interface_type(self.texture_type)
        
    def _set_interface_type(self: Self, tex_type: TextureType) -> None:
        self.texture_type = tex_type
        if self.texture_type == TextureType.IMAGE:
            self.file_selector.set_text(self.texture.filepath)
            self.file_selector.setVisible(True)
            self.constant_color.setVisible(False)
            utils.set_button_icon(self.swap_type_btn, 'palette')
            self.swap_type_btn.setToolTip('From constant color...')
        else:
            self.constant_color.set_from_color(self.texture.constant_color)
            self.constant_color.setVisible(True)
            self.file_selector.setVisible(False)
            utils.set_button_icon(self.swap_type_btn, 'file_image')
            self.swap_type_btn.setToolTip('Load from image...')
            
    def get_texture(self: Self) -> None:
        if self.texture_type == TextureType.IMAGE:
            return Texture.from_image(self.file_selector.current_filepath())
        else: return Texture.from_color(self.constant_color.as_color())
            
class FloatTextureWidget(W.QWidget):
    def __init__(
        self:    Self,
        texture: Texture,
        minimum: float = 0.0,
        maximum: float = 1.0,
    ) -> None:
        super().__init__()
        self.setLayout(W.QHBoxLayout())
        self.layout().setContentsMargins(0, 0, 0, 0)
        
        self.constant = SpinboxSlider(
            minimum, maximum, texture.constant_color.r()
        )
        self.file_selector = FileSelector()
        
        self.layout().addWidget(self.constant)
        self.layout().addWidget(self.file_selector)
        
        self.swap_type_btn = W.QPushButton()
        self.swap_type_btn.setStyleSheet('padding: 3px, 3px;')
        self.swap_type_btn.clicked.connect(
            lambda : self._set_interface_type(
                TextureType.CONSTANT 
                if self.texture_type == TextureType.IMAGE
                else TextureType.IMAGE
            )
        )
        self.layout().addWidget(self.swap_type_btn)
        
        self.texture = texture
        self.texture_type = self.texture.type
        self._set_interface_type(self.texture_type)
        
    def _set_interface_type(self: Self, tex_type: TextureType) -> None:
        self.texture_type = tex_type
        if self.texture_type == TextureType.IMAGE:
            self.file_selector.set_text(self.texture.filepath)
            self.file_selector.setVisible(True)
            self.constant.setVisible(False)
            utils.set_button_icon(self.swap_type_btn, 'hash')
            self.swap_type_btn.setToolTip('From constant color...')
        else:
            self.constant.set_value(self.texture.constant_color.r())
            self.constant.setVisible(True)
            self.file_selector.setVisible(False)
            utils.set_button_icon(self.swap_type_btn, 'file_image')
            self.swap_type_btn.setToolTip('Load from image...')
            
    def get_texture(self: Self) -> None:
        if self.texture_type == TextureType.IMAGE:
            return Texture.from_image(self.file_selector.current_filepath())
        else: 
            value = self.constant.get_value()
            return Color(value, value, value)
        
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
# Lambertian
###############################################################################

class MatSettingsLambertian(MatSettings):
    def __init__(self: Self) -> None:
        super().__init__()
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        self.albedo = VectorWidget(
            (1.0, 1.0, 1.0), min_value=0.0, max_value=1.0
        )
        layout.addRow('Albedo', self.albedo)
        self.setLayout(layout)

    def get_material(self: Self) -> Material:        
        raise NotImplementedError

###############################################################################
# PBR METAL ROUGHNESS
###############################################################################

class MatSettingsPBR(MatSettings):
    def __init__(self: Self, material: Material) -> None:
        super().__init__()
        
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        
        
        self.albedo = ColorTextureWidget(material.get_tracked_texture(0))
        layout.addRow('Albedo', self.albedo)
        
        self.roughness = FloatTextureWidget(material.get_tracked_texture(1))
        layout.addRow('Roughness', self.roughness)
        
        self.metallic = FloatTextureWidget(material.get_tracked_texture(2))
        layout.addRow('Metallic', self.metallic)
        
        self.emission = ColorTextureWidget(material.get_tracked_texture(3))
        layout.addRow('Emission', self.emission)
        self.setLayout(layout)

    def get_material(self: Self) -> Material.PBR:        
        return Material.PBR(
            self.albedo.get_texture(),
            self.roughness.get_texture(),
            self.metallic.get_texture(),
            self.emission.get_texture()
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



