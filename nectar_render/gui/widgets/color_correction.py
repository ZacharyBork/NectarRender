from typing import Self

from PySide6 import QtWidgets as W

from nectar_render import TonemapMethod
from nectar_render.gui.bridge import Bridge

class ColorCorrection:
    def __init__(self: Self, settings: W.QGroupBox) -> None:
        self.settings = settings
        f = settings.findChild
        
        self.enable_tonemap = f(W.QCheckBox, 'enable_tonemap')
        self.enable_tonemap.stateChanged.connect(self.set_tonemap_enabled)
        
        self.tm_method = f(W.QComboBox, 'tonemap_method')
        self.tm_method.addItems(['Reinhard', 'Reinhard Extended', 'ACES'])
        self.tm_method.currentTextChanged.connect(self.set_tonemap_method)
        
        self.tm_alpha = f(W.QSlider, 'tonemap_alpha')
        self.tm_alpha.valueChanged.connect(self.set_tonemap_alpha)
        
        self.linear_to_gamma = f(W.QCheckBox, 'linear_to_gamma')
        self.linear_to_gamma.stateChanged.connect(self.set_linear_to_gamma)
        
        self.tm_white_pt = f(W.QDoubleSpinBox, 'tonemap_white_point')
        self.tm_white_pt.valueChanged.connect(self.set_tonemap_white_point)
        
        self.tm_blend_percent = f(W.QLabel, 'tonemap_blend_percent')
        
        self.enable_white_balance = f(W.QCheckBox, 'enable_white_balance')
        self.enable_white_balance.stateChanged.connect(
            self.set_white_balance_enabled
        )
        self.wb_temp = f(W.QSlider, 'wb_temp')
        self.wb_temp_label = f(W.QLabel, 'wb_temp_label')
        self.wb_temp.valueChanged.connect(self.set_wb_temp)
        self.wb_tint = f(W.QSlider, 'wb_tint')
        self.wb_tint_label = f(W.QLabel, 'wb_tint_label')
        self.wb_tint.valueChanged.connect(self.set_wb_tint)

    def set_tonemap_enabled(self: Self, enabled: bool) -> None:
        Bridge.stream_config.apply_tonemapping = enabled
        Bridge.update_stream_config()

    def set_tonemap_method(self: Self, method: str) -> None:
        match method:
            case 'Reinhard':          method = TonemapMethod.REINHARD
            case 'Reinhard Extended': method = TonemapMethod.REINHARD_EXTENDED
            case 'ACES':              method = TonemapMethod.ACES     
        Bridge.stream_config.tm_method = method
        Bridge.update_stream_config()

    def set_tonemap_alpha(self: Self, value: int) -> None:
        self.tm_blend_percent.setText(f'{value}%')
        Bridge.stream_config.tm_alpha = float(value) / 100.0
        Bridge.update_stream_config()

    def set_linear_to_gamma(self: Self, value: bool) -> None:
        Bridge.stream_config.linear_to_gamma = value
        Bridge.update_stream_config()
        
    def set_tonemap_white_point(self: Self, value: float) -> None:
        if (value <= 0.0): return
        Bridge.stream_config.tm_white_point = value
        Bridge.update_stream_config()
        
    def set_white_balance_enabled(self: Self, value: bool) -> None:
        Bridge.stream_config.apply_white_balance = value
        Bridge.update_stream_config()
        
    def set_wb_temp(self: Self, value: int) -> None:
        self.wb_temp_label.setText(f'{value}k')
        Bridge.stream_config.wb_temperature = float(value)
        Bridge.update_stream_config()
        
    def set_wb_tint(self: Self, value: int) -> None:
        v = float(value) / 100.0
        self.wb_tint_label.setText(f'{v}')
        Bridge.stream_config.wb_tint = v
        Bridge.update_stream_config()


