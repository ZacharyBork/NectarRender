from typing  import Self
from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt, QObject

from nectar_render.gui import utils

class CollapsibleMenu(W.QFrame):
    def __init__(
        self:         Self, 
        name:         str = '',
        start_closed: bool = True,
        parent:       QObject | None = None
    ) -> None:
        super().__init__(parent=parent)
        self.name = name
        self.is_open = not start_closed

        self.setLayout(W.QVBoxLayout())
        self.layout().setAlignment(Qt.AlignmentFlag.AlignTop)
        self.layout().setSpacing(0)
        self.layout().setContentsMargins(0, 0, 0, 0)
        
        self._build_toggle_btn()
        self._build_label()
        self._build_header()
        self._build_menu_area()
        
#### INITIALIZATION ###########################################################
        
    def _build_toggle_btn(self: Self) -> None:
        self.toggle = W.QPushButton('')
        self.toggle.setSizePolicy(
            W.QSizePolicy.Policy.Maximum,
            W.QSizePolicy.Policy.Preferred
        )
        self.toggle.clicked.connect(self._toggle_visibility)
        utils.set_button_icon(
            self.toggle, 'caret_down' if self.is_open else 'caret_right'
        )
        
    def _build_label(self: Self) -> None:
        self.label = W.QLabel(self.name)
        self.label.setStyleSheet('background: none; padding: 0px;')
        
    def _build_header(self: Self) -> None:
        self.header = W.QPushButton('')
        self.header.clicked.connect(self._toggle_visibility)
        self.header.setLayout(W.QHBoxLayout())
        self.header.layout().addWidget(self.toggle)
        self.header.layout().addWidget(self.label)
        self.header.setStyleSheet(
            'background-color: rgba(70, 70, 70, 75);'
            'padding: 10px;'
            'border: none;'
            'border-radius: 6px;'
        )
        self.layout().addWidget(self.header)
    
    def _build_menu_area(self: Self) -> None:
        self.menu_area = W.QFrame()
        layout = W.QVBoxLayout()
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)
        
        self.menu_area.setLayout(layout)
        self.menu_area.setVisible(self.is_open)
        # self.menu_area.setStyleSheet(
        #     'background-color: rgba(70, 70, 70, 50);'
        #     'border-radius: 6px;'
        # )
        self.layout().addWidget(self.menu_area)
    
#### UTILITIES ################################################################

    def add_widget(self: Self, widget: W.QWidget) -> None:
        self.menu_area.layout().addWidget(widget)
        
#### CALLBACKS ################################################################
        
    def _toggle_visibility(self: Self) -> None:
        self.is_open = not self.is_open
        self.menu_area.setVisible(self.is_open)
        utils.set_button_icon(
            self.toggle, 'caret_down' if self.is_open else 'caret_right'
        )
        self.toggle.setStyleSheet(
            'background-color: #ff5e3a;' if self.is_open else
            'background-color: #1f2128;'
        )
        
    
        
            
        
    



