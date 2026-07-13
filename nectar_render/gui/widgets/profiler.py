from typing import Self

from PySide6 import QtWidgets as W
from PySide6.QtCore import QTimer, QPropertyAnimation, QEasingCurve

from nectar_render import CUDAProfiler


class Profiler:
    def __init__(self: Self, profiler_tab: W.QWidget) -> None:        
        self.tab  = profiler_tab
        self.find = self.tab.findChild
        
        self.enabled = self.find(W.QCheckBox, 'profiler_enabled')
        self.enabled.checkStateChanged.connect(self._enabled_callback)
        self.interval = self.find(W.QSpinBox,  'profiler_interval')
        self.interval.valueChanged.connect(self._interval_callback)
        
        self.cuda_mem_used  = self.find(W.QLabel, 'cuda_mem_used')
        self.cuda_mem_free  = self.find(W.QLabel, 'cuda_mem_free')
        self.cuda_mem_total = self.find(W.QLabel, 'cuda_mem_total')
        
        self.update_timer: QTimer | None = None
        self._build_timer()
        
        self.progress_bar = self.find(W.QProgressBar, 'profiler_progress')
        self.progress_anim = QPropertyAnimation(self.progress_bar, b'value', self.tab)
        self.progress_anim.setStartValue(0)
        self.progress_anim.setEndValue(100)
        self.progress_anim.setEasingCurve(QEasingCurve.Type.Linear)
        self.progress_anim.setDuration(self.get_interval())
        
        if self.is_enabled(): self._start_timer()

    def update(self: Self) -> None:
        meminfo = CUDAProfiler.get_cuda_meminfo()
        mb = lambda bytes : round(float(bytes) / 1048576.0, 2)
        self.cuda_mem_used.setText(f'{mb(meminfo.used)}')
        self.cuda_mem_free.setText(f'{mb(meminfo.free)}')
        self.cuda_mem_total.setText(f'{mb(meminfo.total)}')
        
        self.progress_anim.stop()
        self.progress_anim.start()

    def get_interval(self: Self) -> int: return self.interval.value()
    def is_enabled(self: Self) -> bool: return self.enabled.isChecked()
        
    def _build_timer(self: Self) -> None:
        if self.update_timer is not None: self.update_timer.deleteLater()
            
        self.update_timer = QTimer(self.tab)
        self.update_timer.setInterval(self.get_interval())
        self.update_timer.timeout.connect(self.update)
        
    def _start_timer(self: Self) -> None: 
        self.progress_anim.start()
        self.update_timer.start()
        
    def _stop_timer(self: Self) -> None: 
        self.update_timer.stop()
        self.progress_anim.setCurrentTime(0)
        self.progress_anim.stop() 
        
        self._build_timer()
        
    def _enabled_callback(self: Self) -> None:
        if not self.is_enabled(): 
            self._stop_timer()
            self.cuda_mem_used.setText('')
            self.cuda_mem_free.setText('')
            self.cuda_mem_total.setText('')
        else: 
            self.update()
            self._start_timer()
        
    def _interval_callback(self: Self, value: int) -> None:
        self.update_timer.setInterval(self.get_interval())
        self.progress_anim.setDuration(self.get_interval())
            
            
            
