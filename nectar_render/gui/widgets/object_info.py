from typing import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, QObject, Slot, Signal

from nectar_render import SceneInterface, Color, MaterialType, Material as M
from nectar_render.gui.bridge import Bridge
from nectar_render.gui.widgets.materials import MatSettingsPBR
from nectar_render.gui.widgets.vector import VectorWidget
from nectar_render.gui.widgets.xform_controller import XformController

class MaterialSettings(W.QGroupBox):
    def __init__(self: Self) -> None:
        super().__init__()        
        
        
        self.setTitle('Material')
        layout = W.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)
        
        self.material_type = W.QComboBox()
        self.material_type.addItem('Lambertian')
        self.material_type.addItem('PBR')
        self.material_type.addItem('Dielectric')
        self.material_type.addItem('Emissive')
        layout.addWidget(self.material_type)
        
        
        
        mat = Bridge.scene_interface.get_material()
        match mat.material_type():
            case MaterialType.Lambertian: 
                self.material_type.setCurrentIndex(0)
                            
            case MaterialType.PBR:
                self.material_type.setCurrentIndex(1)
                self.core_settings = MatSettingsPBR(mat)
                layout.addWidget(self.core_settings)
            case MaterialType.Dielectric:
                self.material_type.setCurrentIndex(2)
            
            case MaterialType.Emissive:
                self.material_type.setCurrentIndex(3)
            
            case MaterialType.Isotropic:
                self.material_type.setCurrentIndex(4)

        self.update_btn = W.QPushButton('Update')
        self.update_btn.clicked.connect(self.update_material)
        layout.addWidget(self.update_btn)
        
        
    def update_material(self: Self) -> None:
        mat = self.core_settings.get_material()
        Bridge.scene_interface.set_material(mat)
        # Bridge.queue_function(
        #     lambda : Bridge.scene_interface.update_material(mat)
        # )


class ObjectInfo(QObject):
    close_signal = Signal()

    def __init__(self: Self, settings: W.QTabWidget) -> None:
        super().__init__()
        self.setObjectName('object_info')
        self.is_enabled = True
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
        self.is_enabled = True
        
    def destroy(self: Self) -> None:
        self.settings.setTabVisible(0, False)
        self._clear()
        self.is_enabled = False            

    def _clear(self: Self) -> None:
        def _delete_widgets(layout):
            while layout.count():
                child = layout.takeAt(0)
                if   child.widget() is not None: child.widget().deleteLater()
                elif child.layout() is not None:
                    self._delete_widgets(child.layout())
        
        _delete_widgets(self.root)



