from typing import Self

import numpy as np
from PIL import Image
from pathlib import Path
from dataclasses import dataclass, field

from PySide6        import QtWidgets as W
from PySide6.QtCore import Qt, Slot, QPointF
from PySide6.QtGui  import QKeyEvent, QMouseEvent, QImage, QPixmap

from nectar_render import Vector3, SceneInterface, CameraParams
from nectar_render.gui.widgets.object_info import ObjectInfo
from nectar_render.gui.bridge import Bridge

###############################################################################
# UTILITIES
###############################################################################

@dataclass
class FrameBuffer:
    data: np.ndarray
    
    @property
    def C(self: Self) -> int: return self.data.shape[2]
    @property
    def H(self: Self) -> int: return self.data.shape[0]
    @property
    def W(self: Self) -> int: return self.data.shape[1]
    @property
    def strides(self: Self) -> int: return self.data.strides[0]

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
        
        self._build_image_label()
    
#### INITIALIZATION ###########################################################
        
    def _build_image_label(self: Self) -> None:
        self.image_label = W.QLabel()
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter) 
        
        self.setLayout(W.QVBoxLayout())
        self.layout().addWidget(self.image_label)

#### KEYPRESS UTILITIES #######################################################

    def keyPressEvent(self: Self, event: QKeyEvent) -> None:
        if event.key() == Qt.Key.Key_Escape:
            if self.object_info is not None:
                Bridge.queue_function(lambda : self.object_info.destroy())
                
        if event.isAutoRepeat(): return
        self._held_keys.add(event.key())

    def keyReleaseEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat():
            return
        self._held_keys.discard(event.key())

    def is_held(self: Self, key: Qt.Key) -> bool:
        return key in self._held_keys
    
#### SCENE INTERACTION ########################################################
    
    def _handle_scene_interaction(self: Self, click_pos: QPointF) -> None:
        self.object_info.destroy()
        
        size = self.size()
        Bridge.instance.ENGINE.screen_space_ray(
            click_pos.x() / size.width(), click_pos.y() / size.height()
        )
        self.object_info.build()
      
#### MOUSE UTILITIES ##########################################################

    def mousePressEvent(self, event: QMouseEvent) -> None:
        if event.button() == Qt.MouseButton.RightButton:
            self._looking = True
            self._curr_mouse_pos = self._prev_mouse_pos = event.position()
        elif event.button() == Qt.MouseButton.LeftButton:
            self._handle_scene_interaction(event.position())

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
    
    def update_image(self: Self) -> None:
        data = np.ascontiguousarray(Bridge.instance.get_data())
        self.buffer = FrameBuffer(data)
        
        qimg = QImage(
            self.buffer.data, self.buffer.W, self.buffer.H, 
            self.buffer.strides, QImage.Format_RGB888
        )
        qimg = qimg.copy()
        pixmap = QPixmap.fromImage(qimg)
        self.image_label.setPixmap(pixmap)
        
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
        


