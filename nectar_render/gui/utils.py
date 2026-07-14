from pathlib import Path

import PySide6.QtWidgets as W
from PySide6.QtCore import QSize
from PySide6.QtGui  import QIcon

def set_button_icon(
    button:   W.QPushButton,
    filename: str, 
    size:     tuple[int, int] = (16, 16)
) -> None:
    root = Path(__file__).parent.resolve()
    file = root / f'resource/icons/{filename}.png'
    button.setIcon(QIcon(file.as_posix()))
    button.setIconSize(QSize(*size))

