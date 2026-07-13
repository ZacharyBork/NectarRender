from typing import Self

import threading
import numpy as np
from PIL import Image
from pathlib import Path
from dataclasses import dataclass, field

from PySide6        import QtWidgets
from PySide6.QtCore import Qt, QObject, Signal, Slot, QPointF
from PySide6.QtGui  import QKeyEvent, QMouseEvent, QImage, QPixmap

from nectar_render import RenderEngine, Scene, Vector3, HitRecord
from nectar_render.gui.widgets.object_info import ObjectInfo
from nectar_render.gui.bridge import RenderBridge, BridgeReset

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

class ViewportWidget(QtWidgets.QLabel):
        
    def __init__(
        self:         Self, 
        bridge:       RenderBridge, 
        cam_settings: QtWidgets.QGroupBox
    ) -> None:
        super().__init__()
        
        self.bridge = bridge
        self.buffer: FrameBuffer = None
        
        self.cam_settings = cam_settings
        self.cam_movement_speed = 0.05
        self.cam_look_sensitivity = 0.15
        
        self._looking = False
        self._curr_mouse_pos: QPointF | None = None
        self._prev_mouse_pos: QPointF | None = None
        
        self._cam_data = CameraUpdateInfo()
        self._cam_data.prev = CameraUpdateInfo()
        
        self.object_info: ObjectInfo | None = None
        
        self.bridge.signals.paused.connect(self._run_camera_update)
        
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
        self._held_keys: set[Qt.Key] = set()
        
        self._build_image_label()
    
#### INITIALIZATION ###########################################################
        
    def _build_image_label(self: Self) -> None:
        self.image_label = QtWidgets.QLabel()
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter) 
        
        self.setLayout(QtWidgets.QVBoxLayout())
        self.layout().addWidget(self.image_label)

#### KEYPRESS UTILITIES #######################################################

    def keyPressEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat(): return
        self._held_keys.add(event.key())

    def keyReleaseEvent(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat():
            return
        self._held_keys.discard(event.key())

    def is_held(self: Self, key: Qt.Key) -> bool:
        return key in self._held_keys
    
#### MOUSE UTILITIES ##########################################################
    
    def _handle_scene_interaction(self: Self, click_pos: QPointF) -> None:
        if self.object_info is not None:
            self.object_info.deleteLater()
        
        size = self.size()
        interface = self.bridge.ENGINE.screen_space_ray(
            click_pos.x() / size.width(), click_pos.y() / size.height()
        )
        self.object_info = ObjectInfo(self.bridge, interface, self.image_label)
        self.object_info.show()
        self.object_info.raise_()
        
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
            QtWidgets.QDoubleSpinBox, name
        ).value()
        
        self._cam_data.focal_length   = get_value('focal_length')
        self._cam_data.focus_distance = get_value('focus_distance')        
        self._cam_data.aperture       = get_value('aperture')
        self._cam_data.sensor_width   = get_value('sensor_width')
        self._cam_data.shutter_speed  = get_value('shutter_speed')
            
    def update_camera(self: Self) -> None:
        if not self.bridge.is_rendering(): return
        
        self._update_cam_transforms()
        self._parse_camera_settings()
        if self._cam_data.should_update():
            if self.bridge.ENGINE.is_rendering():
                self.bridge.request_pause()
            else: self._run_camera_update()
      
    @Slot()
    def _run_camera_update(self: Self) -> None:
        if not self._cam_data.should_update(): return
                
        with BridgeReset():
            self.bridge.camera.update(
                Vector3(*self._cam_data.delta_p) * self.cam_movement_speed, 
                Vector3(*self._cam_data.delta_r) * self.cam_look_sensitivity,
                self._cam_data.focal_length,
                self._cam_data.focus_distance,
                self._cam_data.aperture,
                self._cam_data.sensor_width,
                self._cam_data.shutter_speed
            )
        
        self._cam_data.reset()
        
#### IMAGE UTILITIES ##########################################################
    
    def update_image(self: Self) -> None:
        self.buffer = FrameBuffer(np.ascontiguousarray(self.bridge.get_data()))
        
        qimg = QImage(
            self.buffer.data, self.buffer.W, self.buffer.H, 
            self.buffer.strides, QImage.Format_RGB888
        )
        qimg = qimg.copy()
        pixmap = QPixmap.fromImage(qimg)
        self.image_label.setPixmap(pixmap)
        
    def save_image(self: Self) -> None:
        if self.buffer is None: return
        
        fp, _ = QtWidgets.QFileDialog.getSaveFileName()
        if not fp: 
            print('No file path selected.')
            return
        
        fp = Path(fp).resolve()
        if not fp.parent.exists():
            print(f'Unable to locate parent directory at '
                  f'{fp.parent.as_posix()}')
            
        Image.fromarray(self.buffer.data).save(fp)
        


