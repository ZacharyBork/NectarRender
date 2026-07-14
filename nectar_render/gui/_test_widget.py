import sys
from typing  import Self
from pathlib import Path

from PySide6 import QtWidgets as W
from PySide6.QtCore import QObject, QTimer

from nectar_render.gui.widgets import XformController

class WidgetTester(QObject):    
    def __init__(self: Self) -> None:
        super().__init__()

        self.update_timer = QTimer(self)
        self.update_timer.setInterval(16)
        self.update_timer.timeout.connect(self.update_timer_elapsed)
        
    def update_timer_elapsed(self: Self) -> None:
        pass
    
    def _set_stylesheet(self: Self) -> None:
        path = Path(__file__).parent.resolve() / 'resource/stylesheet.qss'
        if not path.exists():
            raise FileNotFoundError(
                f'Unable to locate stylesheet: {path.as_posix()}'
            )
        with open(path.as_posix(), 'r') as file:
            self.app.setStyleSheet(file.read())

    def run(self: Self) -> None:
        self.app = W.QApplication(sys.argv)
        self._set_stylesheet()

        WIDGET = XformController()


        self.update_timer.start()
        WIDGET.show()
        
        sys.exit(self.app.exec())


if __name__ == '__main__':
    WidgetTester().run()
