from typing import Self

from PySide6 import QtWidgets as W


###############################################################################
# PROGRESS BAR WIDGET
###############################################################################

class ProgressBar(W.QWidget):
    
    def __init__(
        self:   Self, 
        layout: W.QLayout
    ) -> None:
        super().__init__()
        self.progress_bar = W.QProgressBar()
        self.sample_counter = W.QLabel('0/0 samples')
        
        hbox = W.QHBoxLayout()
        hbox.setContentsMargins(3, 3, 3, 3)
        hbox.setSpacing(3)
        
        hbox.addWidget(self.progress_bar)
        hbox.addWidget(self.sample_counter)
        
        self.setLayout(hbox)
        layout.addWidget(self)

    def update(self: Self, current: int, total: int) -> None:
        self.progress_bar.setValue(int(current / max(1, total) * 100.0))
        self.sample_counter.setText(f'{current}/{total} samples')
        
    def reset(self: Self) -> None:
        self.progress_bar.setValue(0)
        self.sample_counter = W.QLabel('0/0 samples')
        
        
