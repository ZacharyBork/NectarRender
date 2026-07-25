import sys
from typing  import Self
from pathlib import Path
from dataclasses import dataclass, field

from PySide6        import QtWidgets as W
from PySide6.QtCore import Qt, Slot, QPointF, QObject
from PySide6.QtGui  import (
    QKeyEvent, QMouseEvent, QImage, QPixmap, QResizeEvent
)
from nectar_render import Vector3, CameraParams
from nectar_render.gui.bridge  import Bridge

###############################################################################
# UPDATE DATACLASS
###############################################################################

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
            any(self.delta_p) != 0.0
         or any(self.delta_r) != 0.0
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
# CONTROLLER CLASS
###############################################################################

class CameraController:
    def __init__(self: Self, camera_settings: W.QGroupBox) -> None:
        super().__init__()
                
        self.settings = camera_settings
        
        self._looking = False
        self._curr_mouse_pos: QPointF | None = None
        self._prev_mouse_pos: QPointF | None = None
        
        self._cam_data = CameraUpdateInfo()
        self._cam_data.prev = CameraUpdateInfo()

        self._held_keys: set[Qt.Key] = set()
        
        self._camera_params: CameraParams | None = None

    @property
    def params(self: Self) -> CameraParams | None: 
        return self._camera_params
                
#### KEYPRESS UTILITIES #######################################################

    def key_press(self: Self, event: QKeyEvent) -> None:
        self._held_keys.add(event.key())

    def key_release(self: Self, event: QKeyEvent) -> None:
        if event.isAutoRepeat(): return
        self._held_keys.discard(event.key())

    def is_held(self: Self, key: Qt.Key) -> bool:
        return key in self._held_keys
        
#### MOUSE UTILITIES ##########################################################

    def mouse_press(self, event: QMouseEvent) -> None:
        if event.button() == Qt.MouseButton.RightButton:
            self._looking = True
            self._curr_mouse_pos = self._prev_mouse_pos = event.position()

    def mouse_move(self, event: QMouseEvent) -> None:
        if self._looking: self._curr_mouse_pos = event.position()

    def mouse_release(self, event: QMouseEvent) -> None:
        self._looking = False

#### CAMERA UTILITIES #########################################################
    
    def update_transforms(self: Self, delta_time: float) -> None:
        dT = delta_time
        if self.is_held(Qt.Key.Key_W): self._cam_data.delta_p[2] -= 1.0 * dT
        if self.is_held(Qt.Key.Key_S): self._cam_data.delta_p[2] += 1.0 * dT
        if self.is_held(Qt.Key.Key_A): self._cam_data.delta_p[0] -= 1.0 * dT
        if self.is_held(Qt.Key.Key_D): self._cam_data.delta_p[0] += 1.0 * dT
        if self.is_held(Qt.Key.Key_Q): self._cam_data.delta_p[1] -= 1.0 * dT
        if self.is_held(Qt.Key.Key_E): self._cam_data.delta_p[1] += 1.0 * dT
        
        if self._looking and self._curr_mouse_pos is not None:
            if self._prev_mouse_pos is not None:
                self._cam_data.delta_r[1] += (
                    self._curr_mouse_pos.x() - self._prev_mouse_pos.x()
                ) * dT
                self._cam_data.delta_r[0] += (
                    self._curr_mouse_pos.y() - self._prev_mouse_pos.y()
                ) * dT
            self._prev_mouse_pos = self._curr_mouse_pos
                
    def _parse_camera_settings(self: Self) -> None:
        get_value = lambda name : self.settings.findChild(
            W.QDoubleSpinBox, name
        ).value()
        
        self._cam_data.focal_length   = get_value('focal_length')
        self._cam_data.focus_distance = get_value('focus_distance')        
        self._cam_data.aperture       = get_value('aperture')
        self._cam_data.sensor_width   = get_value('sensor_width')
        self._cam_data.shutter_speed  = get_value('shutter_speed')
        
    def poll_updates(self: Self) -> bool:
        self._parse_camera_settings()
        
        if not self._cam_data.should_update(): return False
        Bridge.start_if_stopped()
                
        params = Bridge.camera.parameters()
        self._camera_params = CameraParams(
            params.resolution,
            Vector3(*self._cam_data.delta_p), 
            Vector3(*self._cam_data.delta_r),
            params.samples_per_pixel,
            self._cam_data.focal_length,
            self._cam_data.focus_distance,
            self._cam_data.aperture,
            self._cam_data.sensor_width,
            self._cam_data.shutter_speed
        )
        
        self._cam_data.reset()
        return True
        
        # Bridge.queue_function(
        #     lambda : Bridge.camera.update(update_params)
        # )
        # self._cam_data.reset()
            


