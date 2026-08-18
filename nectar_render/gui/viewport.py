from typing import Self

import numpy as np
from PIL import Image
from pathlib import Path
from dataclasses import dataclass, field

from PySide6        import QtWidgets as W
from PySide6.QtCore import Qt, QSize, QPointF
from PySide6.QtGui  import (
    QKeyEvent, QMouseEvent, QImage, QPixmap, QResizeEvent
)

from nectar_render import SceneInterface
from nectar_render.gui.widgets.object_info import ObjectInfo
from nectar_render.gui.widgets.gnomon import GnomonWidget
from nectar_render.gui.bridge   import Bridge
from nectar_render.gui.registry import WidgetRegistry
from nectar_render.gui.camera   import CameraController
from nectar_render.gui.utils    import TimeKeeper

from nectar_render.gui.settings_groups import CameraSettings

###############################################################################
# UTILITIES
###############################################################################

@dataclass
class FrameBuffer:
    data: np.ndarray
    
    _pixmap: QPixmap = field(init=False)
    
    @property
    def C(self: Self) -> int: return self.data.shape[2]
    @property
    def H(self: Self) -> int: return self.data.shape[0]
    @property
    def W(self: Self) -> int: return self.data.shape[1]
    @property
    def strides(self: Self) -> int: return self.data.strides[0]
    @property
    def pixmap(self: Self) -> QPixmap: return self._pixmap
    
    def __post_init__(self: Self) -> None:
        self._pixmap = QPixmap.fromImage(
            QImage(
                self.data, self.W, self.H, self.strides, QImage.Format_RGB888
            ).copy()
        )

class ViewportOverlay(W.QWidget):
    def __init__(self: Self, parent: "ViewportWidget") -> None:
        super().__init__(parent=parent)
        self.set_size(parent.size())
        self.setLayout(W.QVBoxLayout())
        self.layout().setAlignment(Qt.AlignmentFlag.AlignTop)
        self.layout().setContentsMargins(3, 3, 3, 3)
        
        self.engine_state = W.QLabel('Idle   ')
        self.engine_state.setStyleSheet(
            'background-color: rgba(70, 70, 70, 70);'
            'padding: 5px;'
            'border-radius: 5px;'
        )
        self.state_ticker = 0
        frame = W.QFrame()
        frame.setSizePolicy(
            W.QSizePolicy.Policy.Maximum,
            W.QSizePolicy.Policy.Maximum
        )
        frame.setLayout(W.QHBoxLayout())
        frame.layout().setAlignment(Qt.AlignmentFlag.AlignLeft)
        frame.layout().addWidget(self.engine_state)
        
        
        TimeKeeper.seconds[1].timeout.connect(self._check_engine_state)
        
        self.layout().addWidget(frame)
        
    def set_size(self: Self, size: QSize) -> None:
        self.setFixedSize(size)
        
    def _check_engine_state(self: Self) -> None:
        state = Bridge.state_as_string()
        self.state_ticker = (self.state_ticker + 1) % 4
        text = f'{state}' + '.' * self.state_ticker
        self.engine_state.setText(text.ljust(len(state)+4))
    
###############################################################################
# VIEWPORT WIDGET
###############################################################################

class ViewportWidget(W.QLabel):
        
    def __init__(
        self:     Self, 
        settings: W.QTabWidget
    ) -> None:
        super().__init__()
        
        WidgetRegistry.register_viewport(self)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.settings = settings
        
        self.overlay = ViewportOverlay(self)
        self.gnomon = GnomonWidget(self)
        self.gnomon.move(0, self.size().height() - self.gnomon.size().height())
        
        self.buffer: FrameBuffer = None
        self.camera_controller = CameraController(
            settings.findChild(CameraSettings, 'camera_settings')
        )
                        
        self.object_info = ObjectInfo(self.settings)
        self.object_info.close_signal.connect(self.object_info.destroy)
        self.scene_interface: SceneInterface | None = None
                
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self._held_keys: set[Qt.Key] = set()

        TimeKeeper.hertz[30].timeout.connect(self.update_render)
        
#### KEYPRESS UTILITIES #######################################################

    def keyPressEvent(self: Self, event: QKeyEvent) -> None:
        if event.key() == Qt.Key.Key_Escape:
            if not self.object_info.is_enabled: return
            self.object_info.destroy()
            Bridge.scene_interface.disable()
            self.readback_stream()
        else: 
            if event.isAutoRepeat(): return
            self.camera_controller.key_press(event)

    def keyReleaseEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat(): return
        self.camera_controller.key_release(event)
    
#### SCENE INTERACTION ########################################################
    
    def _handle_scene_interaction(
        self:      Self, 
        click_pos: tuple[float, float]
    ) -> None:
        if self.object_info.is_enabled: self.object_info.destroy()
        Bridge.scene_interface.query_scene(*click_pos)
        WidgetRegistry.outliner.set_selected(
            Bridge.scene_interface.get_hit_record().object_id
        )
        self.object_info.build()
      
#### MOUSE UTILITIES ##########################################################

    def _normalize_click_pos(
        self:      Self, 
        click_pos: QPointF
    ) -> tuple[float, float] | None:
        pixmap = self.pixmap()
        if pixmap is None or pixmap.isNull():
            return None

        label_w, label_h = self.width(), self.height()
        pix_w, pix_h = pixmap.width(), pixmap.height()

        scale = min(label_w / pix_w, label_h / pix_h)
        displayed_w = pix_w * scale
        displayed_h = pix_h * scale

        offset_x = (label_w - displayed_w) / 2.0
        offset_y = (label_h - displayed_h) / 2.0

        local_x = click_pos.x() - offset_x
        local_y = click_pos.y() - offset_y

        if local_x < 0 \
        or local_y < 0 \
        or local_x > displayed_w \
        or local_y > displayed_h:
            return None

        return (local_x / displayed_w, local_y / displayed_h)

    def mousePressEvent(self, event: QMouseEvent) -> None:
        if event.button() == Qt.MouseButton.LeftButton:
            gnomon = WidgetRegistry.transform_gnomon
            if gnomon is not None and gnomon.try_begin_drag(event.position()):
                return

            click_pos = self._normalize_click_pos(event.position())
            if click_pos is not None:
                self._handle_scene_interaction(click_pos)
                self.readback_stream()
        elif event.button() == Qt.MouseButton.RightButton:
            self.camera_controller.mouse_press(event)
        
    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        gnomon = WidgetRegistry.transform_gnomon
        if gnomon is not None and gnomon.is_dragging:
            gnomon.continue_drag(event.position())
            return
        if gnomon is not None:
            gnomon.update_hover(event.position())
        self.camera_controller.mouse_move(event)
    
    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        gnomon = WidgetRegistry.transform_gnomon
        if gnomon is not None and gnomon.is_dragging:
            gnomon.end_drag()
            return
        if event.button() == Qt.MouseButton.RightButton:
            self.camera_controller.mouse_release(event)

#### IMAGE UTILITIES ##########################################################
    
    def resizeEvent(self, event: QResizeEvent) -> None:
        super().resizeEvent(event)
        self.update_pixmap()
        self.gnomon.move(0, self.size().height() - self.gnomon.size().height())
        self.overlay.set_size(self.size())
    
    def update_buffer(self: Self) -> None:
        self.buffer = FrameBuffer(Bridge.readback_stream())
    
    def update_pixmap(self: Self) -> None:
        if self.buffer is None \
        or self.buffer.pixmap is None \
        or self.buffer.pixmap.isNull():
            return
        scaled = self.buffer.pixmap.scaled(
            self.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self.setPixmap(scaled)
    
    def readback_stream(self: Self) -> None:
        self.update_buffer()
        self.update_pixmap()   
    
    def update_render(self: Self) -> None:
        if not Bridge.is_rendering: return
        self.readback_stream()
    
    def save_image(self: Self) -> None:
        if self.buffer is None: return
        
        fp, _ = W.QFileDialog.getSaveFileName()
        if not fp: 
            print('No file path selected.')
            return
        
        fp = Path(fp).resolve()
        if not fp.parent.exists():
            print(f'Unable to locate parent directory at '
                  f'{fp.parent.as_posix()}')
            
        Image.fromarray(self.buffer.data).save(fp)
        


