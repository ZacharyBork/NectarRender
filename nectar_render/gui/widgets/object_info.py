from typing import Self

from PySide6 import QtWidgets as W

from nectar_render import HitRecord


class ObjectInfo(W.QFrame):
    def __init__(self: Self, rec: HitRecord, parent: W.QWidget) -> None:
        super().__init__(parent=parent)
        self.rec = rec
        
        layout = W.QHBoxLayout()
        layout.addWidget(W.QLabel(str(rec.d_object_ptr())))
        self.setLayout(layout)
        
        
        
        
    


