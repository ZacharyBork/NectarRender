from typing  import Self
from PySide6 import QtWidgets as W
from PySide6.QtCore import Qt

from nectar_render import TonemapMethod
from nectar_render.gui.bridge import Bridge
from nectar_render.gui.widgets.collapsible_menu import CollapsibleMenu


class ColorCorrectionSettings(CollapsibleMenu):
	def __init__(self: Self) -> None:
		super().__init__(
			name='Color Correction', start_closed=True, parent=None
		)

		settings_frame = W.QFrame()
		settings_frame.setStyleSheet('padding: 2px;')
		
		layout = W.QFormLayout()
		layout.setContentsMargins(0, 0, 0, 0)
		settings_frame.setLayout(layout)

		self.to_gamma = W.QCheckBox('')
		self.to_gamma.setChecked(True)
		image_grp = W.QGroupBox('Image')
		l = W.QFormLayout()
		image_grp.setLayout(l)
		l.addRow('To Gamma', self.to_gamma)
		layout.addWidget(image_grp)

		tonemap_grp = W.QGroupBox('Tonemapping')
		l = W.QFormLayout()
		tonemap_grp.setLayout(l)
		

		self.do_tonemapping = W.QCheckBox()
		self.do_tonemapping.setChecked(True)
		l.addRow('Enable', self.do_tonemapping)
		

		self.tonemapping_method = W.QComboBox()
		self.tonemapping_method.addItems(
			['Reinhard', 'Reinhard Extended', 'ACES']
		)
		self.tonemapping_method.currentTextChanged.connect(
			self.set_tonemap_method
		)

		l.addRow('Method', self.tonemapping_method)

		self.tm_alpha = W.QSlider()
		self.tm_alpha.setOrientation(Qt.Orientation.Horizontal)
		self.tm_alpha.setRange(0, 100)
		self.tm_alpha.valueChanged.connect(self.set_tonemap_alpha)
		self.tm_blend_percent = W.QLabel('100%')
		tm_blend_layout = W.QHBoxLayout()
		tm_blend_layout.addWidget(self.tm_alpha)
		tm_blend_layout.addWidget(self.tm_blend_percent)
		l.addRow('Blend', tm_blend_layout)

		self.tm_white_pt = W.QDoubleSpinBox()
		self.tm_white_pt.setRange(0, 99.99)
		self.tm_white_pt.setValue(1.0)
		self.tm_white_pt.setSingleStep(0.2)
		self.tm_white_pt.valueChanged.connect(self.set_tonemap_white_point)
		l.addRow('White Point', self.tm_white_pt)

		layout.addWidget(tonemap_grp)

		white_balance_grp = W.QGroupBox('White Balance')
		l = W.QFormLayout()
		white_balance_grp.setLayout(l)

		self.enable_white_balance = W.QCheckBox()
		self.enable_white_balance.setChecked(True)
		self.enable_white_balance.stateChanged.connect(
			self.set_white_balance_enabled
		)
		l.addRow('Enable', self.enable_white_balance)
		
		self.wb_temp = W.QSlider()
		self.wb_temp.setOrientation(Qt.Orientation.Horizontal)
		self.wb_temp.setRange(1, 12000)
		self.wb_temp.setValue(6800)
		self.wb_temp.valueChanged.connect(self.set_wb_temp)
		self.wb_temp_label = W.QLabel()
		self.wb_temp_label.setText('6800k')

		wb_temp_layout = W.QHBoxLayout()
		wb_temp_layout.addWidget(self.wb_temp)
		wb_temp_layout.addWidget(self.wb_temp_label)
		l.addRow('Temperature', wb_temp_layout)

		self.wb_tint = W.QSlider()
		self.wb_tint.setOrientation(Qt.Orientation.Horizontal)
		self.wb_tint.setRange(0, 500)
		self.wb_tint.setValue(100)
		self.wb_tint_label = W.QLabel('1.0')
		self.wb_tint.valueChanged.connect(self.set_wb_tint)

		
		wb_tint_layout = W.QHBoxLayout()
		wb_tint_layout.addWidget(self.wb_tint)
		wb_tint_layout.addWidget(self.wb_tint_label)
		l.addRow('Tint', wb_tint_layout)

		layout.addWidget(white_balance_grp)

		self.add_widget(settings_frame)

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


