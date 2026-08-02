from typing  import Self
from pathlib import Path
from PySide6 import QtWidgets as W

from nectar_render.gui import utils

class FileSelector(W.QWidget):
    def __init__(
        self:        Self, 
        placeholder: str = 'Enter filepath...',
        parent:      W.QWidget | None = None
    ) -> None:
        super().__init__(parent=parent)
        layout = W.QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(3)
        self.setLayout(layout)
        
        self.input_field = W.QLineEdit('', placeholderText=placeholder)
        layout.addWidget(self.input_field)
        
        self.editingFinished = self.input_field.editingFinished
        self.textChanged     = self.input_field.textChanged
        
        self.browse_btn = W.QPushButton()
        utils.set_button_icon(self.browse_btn, 'file_magnifying_glass')
        self.layout().addWidget(self.browse_btn)

    def set_text(self: Self, text: str) -> None:
        self.input_field.setText(text)

    def browse_file(
        self:      Self,
        caption:   str = 'Open File',
        start_dir: str = '',
        filter:    str = ...
    ) -> None:
        filepath, _ = W.QFileDialog.getOpenFileName(
            self, 
            caption = caption,
            dir     = start_dir,
            filter  = filter
        )
        if not filepath: return
        filepath = Path(filepath)
        if not filepath.exists():
            raise FileNotFoundError(
                f'Unable to locate input file at path: {filepath.as_posix()}'
            )
        self.browse_btn.setText(filepath.as_posix())
        
    def current_filepath(self: Self) -> str:
        return self.input_field.text()


