from typing  import Self
from pathlib import Path
from PySide6 import QtWidgets as W
from PySide6.QtCore import Signal

from nectar_render.gui import utils
from nectar_render.gui.registry import WidgetRegistry

class FileSelector(W.QWidget):
    path_updated = Signal(str)
    
    def __init__(
        self:           Self, 
        placeholder:    str = 'Enter filepath...',
        dialog_caption: str = 'Open File',
        dialog_dir:     str = '',
        dialog_filter:  str = '',
        parent:      W.QWidget | None = None
    ) -> None:
        super().__init__(parent=parent)
        self.dialog_caption = dialog_caption
        self.dialog_dir     = dialog_dir
        self.dialog_filter  = dialog_filter
        
        layout = W.QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(3)
        self.setLayout(layout)
        
        self.input_field = W.QLineEdit('', placeholderText=placeholder)
        layout.addWidget(self.input_field)
        
        self.input_field.editingFinished.connect(
            lambda : self.path_updated.emit(self.input_field.text())
        )

        self.browse_btn = W.QPushButton()
        self.browse_btn.clicked.connect(self.browse_file)
        utils.set_button_icon(self.browse_btn, 'file_magnifying_glass')
        self.layout().addWidget(self.browse_btn)

    def set_text(self: Self, text: str) -> None:
        self.input_field.setText(text)

    def browse_file(self: Self) -> None:
        filepath, _ = W.QFileDialog.getOpenFileName(
            self,
            self.dialog_caption,
            self.dialog_dir,
            self.dialog_filter
        )
        if not filepath: return
        filepath = Path(filepath)
        if not filepath.exists():
            raise FileNotFoundError(
                f'Unable to locate input file at path: {filepath.as_posix()}'
            )
        
        fp = filepath.as_posix()
        self.input_field.setText(fp)
        self.path_updated.emit(fp)
        
    def current_filepath(self: Self) -> str:
        return self.input_field.text()


