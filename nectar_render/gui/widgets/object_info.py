from typing import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, QObject, Slot, Signal

from nectar_render import SceneInterface, Color, Material as M
from nectar_render.gui.bridge import Bridge
from nectar_render.gui.widgets.materials import MatSettingsPBR
from nectar_render.gui.widgets.xform_controller import (
    XformController, VectorWidget
)

class MaterialSettings(W.QGroupBox):
    def __init__(self: Self) -> None:
        super().__init__()        
        
        
        mat = Bridge.scene_interface.get_material()
        print(mat.material_type())
        
        
        
        
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
        
        self.pbr_settings = MatSettingsPBR()
        layout.addWidget(self.pbr_settings)

        self.update_btn = W.QPushButton('Update')
        self.update_btn.clicked.connect(self.update_material)
        layout.addWidget(self.update_btn)
        
        
    def update_material(self: Self) -> None:
        mat = self.pbr_settings.get_material()
        Bridge.queue_function(
            lambda : Bridge.scene_interface.update_material(mat)
        )


class ObjectInfo(QObject):
    close_signal = Signal()

    def __init__(self: Self, settings: W.QTabWidget) -> None:
        super().__init__()
        self.setObjectName('object_info')
        self.settings = settings
        self.settings.setTabVisible(0, False)
        self.root = settings.findChild(W.QFrame, 'selected_settings').layout()

    def build(self: Self) -> None:
        self.settings.setTabVisible(0, True)
        self.settings.setCurrentIndex(0)
        
        close_btn = W.QPushButton('X')
        close_btn.clicked.connect(lambda : self.close_signal.emit())
        self.root.addWidget(close_btn)
                
        self.root.addWidget(XformController())
        self.root.addWidget(MaterialSettings())
        
    def destroy(self: Self) -> None:
        self.settings.setTabVisible(0, False)
        self._clear()

    def _clear(self: Self) -> None:
        def _delete_widgets(layout):
            while layout.count():
                child = layout.takeAt(0)
                if   child.widget() is not None: child.widget().deleteLater()
                elif child.layout() is not None:
                    self._delete_widgets(child.layout())
        
        _delete_widgets(self.root)



