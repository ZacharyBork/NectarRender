from nectar_render import (
    RenderEngine, Camera, Vector3, Color, Material, Texture, 
    Hittable, Scene, SkyLight, TVDenoiser
)

import sys
from typing  import Self
from pathlib import Path

from PySide6 import QtWidgets
from PySide6.QtCore    import QFile, QObject
from PySide6.QtUiTools import QUiLoader
from PySide6.QtGui import QPixmap
from qimage2ndarray import array2qimage






class Interface(QObject):    
    def __init__(self: Self) -> None:
        super().__init__()
        
        self.engine = RenderEngine(
            camera = Camera(
                resolution   = (1024, 1024),
                position     = (0.0, 0.0, 3.0),
                rotation     = (0.0, 0.0, 0.0),
                focal_length = 2.0
            ),
            samples   = 500,
            max_depth = 10,
            seed      = None
        )
        self.engine.ENGINE.on_frame_finished = self._on_frame_finished
        
        self.scene  = Scene(
            skylight  = SkyLight(),
            hittables = [
                Hittable.SPHERE(
                    Vector3(0.0, 0.0, 0.0), 0.5,
                    Material.LAMBERTIAN(Color.white())
                ),
                Hittable.SPHERE(
                    Vector3(0.0, -100.5, 0.0), 100.0,
                    Material.LAMBERTIAN(Color(1.0, 1.0, 0.0))
                )
            ]
        )

    ### CALLBACKS ###
    
    def _on_frame_finished(self: Self, frame_idx: int) -> None:
        if frame_idx % 10 == 1: 
            data = self.engine.get_data()
            pixmap = QPixmap.fromImage(array2qimage(data))
            self.find(QtWidgets.QLabel, 'image_label').setPixmap(pixmap)

    def render(self: Self) -> None:
        self.engine.render(self.scene)
        

    ### INITIALIZATION ###

    def _init_mainwidget(self: Self) -> QtWidgets.QWidget:
        path = Path(__file__).parent.resolve() / 'resource/mainwidget.ui'
        if not path.exists():
            msg = f'Unable to locate UI file: {path.resolve().as_posix()}'
            raise FileNotFoundError(msg)
        
        file = QFile(path.as_posix())
        file.open(QFile.ReadOnly)
        widget = QUiLoader().load(file)
        file.close()
        return widget

    def _set_stylesheet(self: Self) -> None:
        path = Path(__file__).parent.resolve() / 'resource/stylesheet.qss'
        if not path.exists():
            raise FileNotFoundError(
                f'Unable to locate stylesheet: {path.as_posix()}'
            )
        with open(path.as_posix(), 'r') as file:
            self.app.setStyleSheet(file.read())

    def _init_callbacks(self: Self) -> None:
        self.find(QtWidgets.QPushButton, 'render').clicked.connect(self.render)

    ### ENTRYPOINT ###

    def run(self: Self) -> None:
        self.app = QtWidgets.QApplication(sys.argv)
        self._set_stylesheet()

        self.mainwidget = self._init_mainwidget()
        self.mainwidget.setWindowTitle('NectarRender')
        self.find = self.mainwidget.findChild
        self._init_callbacks()

        self.mainwidget.show()
        sys.exit(self.app.exec())



