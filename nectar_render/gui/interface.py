from nectar_render.pathtracer import ENGINE

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

    ### CALLBACKS ###

    def render(self: Self) -> None:
        ENGINE.render()
        data = ENGINE.get_data()
        pixmap = QPixmap.fromImage(array2qimage(data))
        self.find(QtWidgets.QLabel, 'image_label').setPixmap(pixmap)

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
        
        ENGINE.init(channels=3, resolution=(512, 512))

        self.mainwidget = self._init_mainwidget()
        self.mainwidget.setWindowTitle('NectarRender')
        self.find = self.mainwidget.findChild
        self._init_callbacks()

        self.mainwidget.show()
        sys.exit(self.app.exec())



