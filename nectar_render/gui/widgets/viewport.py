from typing import Self

import numpy as np
from PIL import Image
from pathlib import Path
from dataclasses import dataclass, field

from PySide6        import QtWidgets as W
from PySide6.QtCore import Qt, Slot, QPointF
from PySide6.QtGui  import (
    QKeyEvent, QMouseEvent, QImage, QPixmap, QResizeEvent
)

from nectar_render import Vector3, SceneInterface, CameraParams
from nectar_render.gui.widgets.object_info import ObjectInfo
from nectar_render.gui.bridge import Bridge

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

@dataclass
class CameraUpdateInfo:
    
    delta_p: list[float] = field(default_factory=lambda : [0.0, 0.0, 0.0])
    delta_r: list[float] = field(default_factory=lambda : [0.0, 0.0, 0.0])
    
    focal_length:   float = 3.0
    focus_distance: float = 10.0
    aperture:       float = 0.01
    sensor_width:   float = 2.0
    shutter_speed:  float = 1.0
    
    prev: "CameraUpdateInfo" = None

    def reset(self: Self) -> None:
        self.delta_p = [0.0, 0.0, 0.0]
        self.delta_r = [0.0, 0.0, 0.0]
        
        self.prev.copy(self)
        
    def should_update(self: Self) -> bool:
        return (
            abs(sum(self.delta_p)) > 0.0
         or abs(sum(self.delta_r)) > 0.0
         or not self == self.prev
        )
    
    def copy(self: Self, other: Self) -> None:
        self.focal_length   = other.focal_length
        self.focus_distance = other.focus_distance
        self.aperture       = other.aperture
        self.sensor_width   = other.sensor_width
        self.shutter_speed  = other.shutter_speed

    def __eq__(self: Self, other: Self) -> bool:
        return (self.focal_length   == other.focal_length
            and self.focus_distance == other.focus_distance
            and self.aperture       == other.aperture
            and self.sensor_width   == other.sensor_width
            and self.shutter_speed  == other.shutter_speed)
    
###############################################################################
# VIEWPORT WIDGET
###############################################################################

class ViewportWidget(W.QLabel):
        
    def __init__(
        self:     Self, 
        settings: W.QTabWidget
    ) -> None:
        super().__init__()
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        self.buffer: FrameBuffer = None
        
        self.settings = settings
        self.cam_settings = self.settings.findChild(
            W.QGroupBox, 'camera_settings'
        )
        self.cam_movement_speed = 0.05
        self.cam_look_sensitivity = 0.15
        
        self._looking = False
        self._curr_mouse_pos: QPointF | None = None
        self._prev_mouse_pos: QPointF | None = None
        
        self._cam_data = CameraUpdateInfo()
        self._cam_data.prev = CameraUpdateInfo()
        
        self.object_info = ObjectInfo(self.settings)
        self.object_info.close_signal.connect(
            lambda : Bridge.queue_function(self.object_info.destroy)
        )
        self.scene_interface: SceneInterface | None = None
                
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self._held_keys: set[Qt.Key] = set()
        
#### KEYPRESS UTILITIES #######################################################

    def keyPressEvent(self: Self, event: QKeyEvent) -> None:
        if event.key() == Qt.Key.Key_Escape:
            self.object_info.destroy()
            Bridge.scene_interface.disable()
                
        if event.isAutoRepeat(): return
        self._held_keys.add(event.key())

    def keyReleaseEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat():
            return
        self._held_keys.discard(event.key())

    def is_held(self: Self, key: Qt.Key) -> bool:
        return key in self._held_keys
    
#### SCENE INTERACTION ########################################################
    
    def _handle_scene_interaction(
        self:      Self, 
        click_pos: tuple[float, float]
    ) -> None:
        if self.object_info.is_enabled: self.object_info.destroy()
        Bridge.instance.ENGINE.screen_space_ray(*click_pos)
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
        if event.button() == Qt.MouseButton.RightButton:
            self._looking = True
            self._curr_mouse_pos = self._prev_mouse_pos = event.position()
        elif event.button() == Qt.MouseButton.LeftButton:
            click_pos = self._normalize_click_pos(event.position())
            if click_pos is not None:
                self._handle_scene_interaction(click_pos)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        if self._looking:
            self._curr_mouse_pos = event.position()

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        if event.button() == Qt.MouseButton.RightButton:
            self._looking = False

#### CAMERA UTILITIES #########################################################
    
    def _update_cam_transforms(self: Self) -> None:
        delta_p = self._cam_data.delta_p
        if self.is_held(Qt.Key.Key_W): delta_p[2] -= 1.0
        if self.is_held(Qt.Key.Key_S): delta_p[2] += 1.0
        if self.is_held(Qt.Key.Key_A): delta_p[0] -= 1.0
        if self.is_held(Qt.Key.Key_D): delta_p[0] += 1.0
        if self.is_held(Qt.Key.Key_Q): delta_p[1] -= 1.0
        if self.is_held(Qt.Key.Key_E): delta_p[1] += 1.0
        
        delta_r = self._cam_data.delta_r
        if self._looking and self._curr_mouse_pos is not None:
            if self._prev_mouse_pos is not None:
                delta_r[1] = self._curr_mouse_pos.x()-self._prev_mouse_pos.x()
                delta_r[0] = self._curr_mouse_pos.y()-self._prev_mouse_pos.y()
            self._prev_mouse_pos = self._curr_mouse_pos
                
    def _parse_camera_settings(self: Self) -> None:
        get_value = lambda name : self.cam_settings.findChild(
            W.QDoubleSpinBox, name
        ).value()
        
        self._cam_data.focal_length   = get_value('focal_length')
        self._cam_data.focus_distance = get_value('focus_distance')        
        self._cam_data.aperture       = get_value('aperture')
        self._cam_data.sensor_width   = get_value('sensor_width')
        self._cam_data.shutter_speed  = get_value('shutter_speed')
        
    def update_camera(self: Self) -> None:
        if not Bridge.instance.is_rendering(): return
        
        self._update_cam_transforms()
        self._parse_camera_settings()
        if not self._cam_data.should_update(): return
        
        params = Bridge.instance.camera.parameters()

        update_params = CameraParams(
            params.resolution,
            Vector3(*self._cam_data.delta_p) * self.cam_movement_speed, 
            Vector3(*self._cam_data.delta_r) * self.cam_look_sensitivity,
            params.samples_per_pixel,
            self._cam_data.focal_length,
            self._cam_data.focus_distance,
            self._cam_data.aperture,
            self._cam_data.sensor_width,
            self._cam_data.shutter_speed
        )
        
        Bridge.queue_function(
            lambda : Bridge.instance.camera.update(update_params)
        )
        self._cam_data.reset()
            
#### IMAGE UTILITIES ##########################################################
    
    def resizeEvent(self, event: QResizeEvent) -> None:
        super().resizeEvent(event)
        self.update_pixmap()
    
    def update_buffer(self: Self) -> None:
        data = np.ascontiguousarray(Bridge.instance.get_data())
        self.buffer = FrameBuffer(data)
    
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
        
    def update_render(self: Self) -> None:
        self.update_buffer()
        self.update_pixmap()
    
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
        


