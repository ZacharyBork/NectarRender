from typing  import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt

from nectar_render.python import SceneOutline, SceneNode
from nectar_render.gui    import utils
from nectar_render.gui.bridge   import Bridge
from nectar_render.gui.registry import WidgetRegistry

class Outliner(W.QWidget):
    def __init__(self: Self, parent: W.QWidget | None = None) -> None:
        super().__init__(parent=parent)
        self.setLayout(W.QVBoxLayout())
        self.layout().setContentsMargins(0, 0, 0, 0)
        self.layout().setSpacing(0)
        
        header_label = W.QLabel('Scene Outliner')
        header_label.setStyleSheet(
            'border: none;'
            'border-radius: 0px;'
        )

        header = W.QFrame()
        header.setLayout(W.QHBoxLayout())
        header.layout().addWidget(header_label)
        header.layout().setContentsMargins(3, 3, 3, 3)
        header.setStyleSheet(
            'background-color: #1f2128;'
            'border: 1px solid #2a2d36;'
            'border-radius: 6px;'
            'margin: 0px 0px -10px 0px;'
            'padding: 2px 2px 12px 2px;'
            'color: #e8e9ed;'
        )
        
        self.toggle_btn = W.QPushButton('')
        self.toggle_btn.setFlat(True)
        self.toggle_btn.setStyleSheet(
            'border: none;'
            'border-radius: 0px;'
        )
        self.toggle_btn.setSizePolicy(
            W.QSizePolicy.Policy.Maximum,
            W.QSizePolicy.Policy.Maximum
        )
        self.toggle_btn.clicked.connect(self._toggle_outliner)
        utils.set_button_icon(self.toggle_btn, 'eye', (16, 16))
        header.layout().addWidget(self.toggle_btn)
        
        self.layout().addWidget(header)
        
        self.outline: SceneOutline | None = None
        self.node_map: dict[int, SceneNode] = {}
        
        self.tree = W.QTreeWidget()
        self.tree.setHeaderLabels(['ID', 'Name', 'Type'])
        self.tree.setAlternatingRowColors(True)
        self.tree.setSortingEnabled(True)
        self.tree.sortItems(0, Qt.SortOrder.AscendingOrder)
        
        self.tree.itemDoubleClicked.connect(self._select_tree_item)
        
        self.layout().addWidget(self.tree)
        self.refresh()

    def _select_tree_item(
        self:    Self, 
        item:    W.QTreeWidgetItem,
        _column: int
    ) -> None:
        id_string = item.text(0)
        if (id_string.isnumeric()):
            node = self.node_map[int(id_string)]
            Bridge.scene_interface.select_scene_node(node)
        
    def _toggle_outliner(self: Self) -> None:
        if self.tree.isVisible():
            self.tree.setVisible(False)
            utils.set_button_icon(self.toggle_btn, 'eye_slash', (16, 16))
            self.parent().setSizePolicy(
                W.QSizePolicy.Policy.Preferred,
                W.QSizePolicy.Policy.Maximum
            )
        else:
            self.tree.setVisible(True)
            utils.set_button_icon(self.toggle_btn, 'eye', (16, 16))
            self.parent().setSizePolicy(
                W.QSizePolicy.Policy.Preferred,
                W.QSizePolicy.Policy.Preferred
            )

    def refresh(self: Self) -> None:
        self.tree.clear()
        self.node_map.clear()
        
        objects_root = W.QTreeWidgetItem(['Objects'])
        objects_root.setFirstColumnSpanned(True)
        self.tree.addTopLevelItem(objects_root)
        
        self.outline = Bridge.scene_interface.get_scene_outline()
        for node in self.outline.nodes:
            self.node_map[node.object_id] = node
            item = W.QTreeWidgetItem(
                [str(node.object_id), node.name, node.type_name]
            )
            objects_root.addChild(item)

        objects_root.setExpanded(True)
