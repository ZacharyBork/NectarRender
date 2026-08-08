from typing  import Self
from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt

from nectar_render.python import DenoiseFilterQuality
from nectar_render.gui.bridge   import Bridge
from nectar_render.gui.registry import WidgetRegistry
from nectar_render.gui.widgets  import CollapsibleMenu, SpinboxSlider

class OIDNSettings(W.QWidget):
    def __init__(self: Self, parent: W.QWidget | None = None) -> None:
        super().__init__(parent=parent)

        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(3)
        self.setLayout(layout)
        self.setStyleSheet('padding: 2px;')

        self.clean_auxiliaries = W.QCheckBox('')
        self.clean_auxiliaries.setChecked(True)
        layout.addRow('Clean Aux', self.clean_auxiliaries)
        self.clean_auxiliaries.stateChanged.connect(self.update_denoiser)

        self.quality = W.QComboBox()
        self.quality.addItems(['Fast', 'Balanced', 'High', 'Default'])
        layout.addRow('Quality', self.quality)
        self.quality.currentIndexChanged.connect(self.update_denoiser)

        self.input_scale = SpinboxSlider(0, 10, 1.0, 1 )
        layout.addRow('Input Scale', self.input_scale)
        self.input_scale.updated.connect(self.update_denoiser)

    def update_denoiser(self: Self) -> None:
        Bridge.stream_config.denoise_clean_auxiliaries = (
            self.clean_auxiliaries.isChecked()
        )
        match self.quality.currentIndex():
            case 0: q = DenoiseFilterQuality.FAST
            case 1: q = DenoiseFilterQuality.BALANCED
            case 2: q = DenoiseFilterQuality.HIGH
            case 3: q = DenoiseFilterQuality.DEFAULT
        Bridge.stream_config.denoise_quality = q
        Bridge.stream_config.denoise_input_scale = (
            float(self.input_scale.get_value()) / 100.0
        )
        Bridge.update_stream_config()
        Bridge.stream.rebuild_denoiser()
        Bridge.requests.restart()
    

class StreamSettings(CollapsibleMenu):
    def __init__(self: Self) -> None:
        super().__init__(
            name='Stream', start_closed=True, parent=None
        )
        self.setObjectName('stream_settings')
        

        settings_frame = W.QFrame()
        settings_frame.setStyleSheet('padding: 2px;')

        layout = W.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(3)
        settings_frame.setLayout(layout)
        self.add_widget(settings_frame)

        denoise_settings = W.QGroupBox('Denoising')
        denoise_layout   = W.QVBoxLayout()
        denoise_settings.setLayout(denoise_layout)
        layout.addWidget(denoise_settings)

        denoise_header = W.QWidget()
        denoise_header_layout = W.QFormLayout()
        denoise_header_layout.setContentsMargins(0, 0, 0, 6)
        denoise_header_layout.setSpacing(3)
        denoise_header.setLayout(denoise_header_layout)
        denoise_layout.addWidget(denoise_header)

        self.denoise_enable = W.QCheckBox('')
        self.denoise_enable.setChecked(True)
        self.denoise_enable.stateChanged.connect(self.update_denoiser)
        denoise_type   = W.QComboBox()
        denoise_type.addItems(['OIDN'])
        denoise_type.setSizePolicy(
            W.QSizePolicy.Policy.Expanding,
            W.QSizePolicy.Policy.Preferred
        )

        l = W.QHBoxLayout()
        l.setSpacing(0)
        l.setContentsMargins(0, 0, 0, 0)
        l.addWidget(self.denoise_enable)
        l.addWidget(denoise_type)
        denoise_header_layout.addRow('Enable', l)

        self.oidn_settings = OIDNSettings()
        denoise_layout.addWidget(self.oidn_settings)

    def update_denoiser(self: Self) -> None:
        if self.denoise_enable.isChecked():
              Bridge.stream.enable_denoising()
        else: Bridge.stream.disable_denoising()
        Bridge.stream.rebuild_denoiser()
        WidgetRegistry.viewport.readback_stream()



    
