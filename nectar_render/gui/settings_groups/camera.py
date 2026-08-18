from typing  import Self
from PySide6 import QtWidgets as W

from nectar_render.gui.widgets import CollapsibleMenu, VectorWidget

class CameraSettings(CollapsibleMenu):
    def __init__(self: Self) -> None:
        super().__init__(
            name='Camera', start_closed=True, parent=None
        )
        self.setObjectName('camera_settings')

        settings_frame = W.QFrame()
        settings_frame.setStyleSheet('padding: 2px;')

        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(3)
        settings_frame.setLayout(layout)

        self.translation = VectorWidget()
        layout.addRow('Translation', self.translation)

        self.rotation = VectorWidget()
        layout.addRow('Rotation', self.rotation)

        self.focal_length = W.QDoubleSpinBox()
        self.focal_length.setRange(0.01, 99.0)
        self.focal_length.setValue(3.0)
        self.focal_length.setDecimals(4)
        layout.addRow('Focal Length', self.focal_length)
        
        self.focus_distance = W.QDoubleSpinBox()
        self.focus_distance.setRange(0.0001, 512.0)
        self.focus_distance.setValue(10.0)
        self.focus_distance.setDecimals(4)
        layout.addRow('Focus Distance', self.focus_distance)
                
        self.aperture = W.QDoubleSpinBox()
        self.aperture.setRange(0.0001, 32.0)
        self.aperture.setValue(0.01)
        self.aperture.setDecimals(4)
        layout.addRow('Aperture', self.aperture)
        
        self.sensor_width = W.QDoubleSpinBox()
        self.sensor_width.setRange(0.0001, 32.0)
        self.sensor_width.setValue(2.0)
        self.sensor_width.setDecimals(4)
        layout.addRow('Sensor Width', self.sensor_width)
        
        self.shutter_speed = W.QDoubleSpinBox()
        self.shutter_speed.setRange(0.0001, 128.0)
        self.shutter_speed.setValue(1.0)
        self.shutter_speed.setDecimals(4)
        layout.addRow('Shutter Speed', self.shutter_speed)

        self.add_widget(settings_frame)


    
