import sys
from typing  import Self
from pathlib import Path

from PySide6 import QtWidgets as W
from PySide6.QtCore    import Qt, QFile, QObject, Slot
from PySide6.QtGui     import QAction
from PySide6.QtUiTools import QUiLoader

from nectar_render import (
    Skylight, SimpleSkylightConfig, HDRISkylightConfig
)
from nectar_render.gui          import utils
from nectar_render.gui.utils    import TimeKeeper
from nectar_render.gui.bridge   import Bridge
from nectar_render.gui.registry import WidgetRegistry
from nectar_render.gui.widgets import (
    CollapsibleMenu, SpinboxSlider, FileSelector, VectorWidget
)

class SimpleSkylightSettings(W.QWidget):
    def __init__(self: Self) -> None:
        super().__init__()
        layout = W.QFormLayout()
        self.setLayout(layout)
        self.layout().setContentsMargins(0, 0, 0, 0)
        
        self.start_color = VectorWidget([1.0, 1.0, 1.0], 0.0, 1.0)
        self.end_color   = VectorWidget([0.5, 0.7, 1.0], 0.0, 1.0)
        
        self.start_color.updated.connect(self._handle_update)
        self.end_color.updated.connect(self._handle_update)
        
        layout.addRow('Start Color', self.start_color)
        layout.addRow('End Color', self.end_color)
        
        self.skylight: Skylight = Bridge.scene_interface.get_skylight()
        self.cfg = self.skylight.config_simple()
        
    def _handle_update(self: Self) -> None:
        self.cfg.start = self.start_color.as_color()
        self.cfg.end   = self.end_color.as_color()
        Bridge.scene_interface.request_skylight_update()
        Bridge.start_if_idle()


class HDRISkylightSettings(W.QWidget):
    def __init__(self: Self) -> None:
        super().__init__()
        layout = W.QFormLayout()
        self.setLayout(layout)
        self.layout().setContentsMargins(0, 0, 0, 0)
        
        file_selector = FileSelector(dialog_filter='*.hdr')
        file_selector.path_updated.connect(self._handle_filepath_update)
        layout.addRow('HDRI File', file_selector)
        
        rotation = SpinboxSlider(0.0, 360.0, 0.0)
        rotation.updated.connect(self._handle_rotation)
        layout.addRow('Rotation', rotation)
        
        self.skylight: Skylight = Bridge.scene_interface.get_skylight()
        self.cfg = self.skylight.config_hdri()
        
    def _handle_filepath_update(self: Self, path: str) -> None:
        self.skylight.load_hdri_file(path)
        Bridge.scene_interface.request_skylight_update()
        Bridge.start_if_idle()
        
    def _handle_rotation(self: Self, value: float) -> None:
        self.cfg.rotation = value
        Bridge.scene_interface.request_skylight_update()
        Bridge.start_if_idle()
        

class SkylightSettings(CollapsibleMenu):
    def __init__(self: Self) -> None:
        super().__init__(
            name='Skylight', start_closed=True, parent=None
        )

        settings_frame = W.QFrame()
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        settings_frame.setLayout(layout)
        settings_frame.setContentsMargins(0, 0, 0, 0)
        
        self.simple_settings = SimpleSkylightSettings()
        self.simple_settings.setVisible(False)
                
        self.hdri_settings = HDRISkylightSettings()
        self.hdri_settings.setVisible(False)
        
        type_widget = W.QWidget()
        type_layout = W.QFormLayout()
        type_widget.setLayout(type_layout)
        skylight_type = W.QComboBox()
        skylight_type.addItems(['None', 'Simple', 'HDRI'])
        skylight_type.setCurrentIndex(1)
        skylight_type.currentTextChanged.connect(self._handle_type)
        type_layout.addRow('Type', skylight_type)
        
        layout.addWidget(type_widget)
        layout.addWidget(self.simple_settings)
        layout.addWidget(self.hdri_settings)
        self.add_widget(settings_frame)

    def _handle_type(self: Self, current_text: str) -> None:
        self.simple_settings.setVisible(current_text=='Simple')
        self.hdri_settings.setVisible(current_text=='HDRI')
        
        match current_text:
            case 'None':
                Bridge.scene_interface.swap_skylight(Skylight())
                Bridge.start_if_idle()
            case 'Simple':
                Bridge.scene_interface.swap_skylight(Skylight.simple())
                Bridge.start_if_idle()
            case 'HDRI': 
                Bridge.scene_interface.swap_skylight(Skylight.hdri())
                Bridge.start_if_idle()
        

