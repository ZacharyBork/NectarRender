from typing import Self
from pathlib import Path
from dataclasses import dataclass, field

import PySide6.QtWidgets as W
from PySide6.QtCore import QObject, QSize, QTimer, QElapsedTimer
from PySide6.QtGui  import QIcon

def set_button_icon(
    button:   W.QPushButton,
    filename: str, 
    size:     tuple[int, int] = (16, 16)
) -> None:
    root = Path(__file__).parent.resolve()
    file = root / f'resource/icons/{filename}.png'
    button.setIcon(QIcon(file.as_posix()))
    button.setIconSize(QSize(*size))


class TimeKeeper:
    hertz: dict[int, QTimer] = field(default_factory=lambda : {
        30: None, 60: None
    })
    seconds: dict[int, QTimer] = field(default_factory=lambda : {
        1: None, 5: None, 10: None, 30: None, 60: None
    })
    
    _dT_timer: QElapsedTimer = None
    _frame_delta: float = 0.0
    
    @staticmethod
    def set_owner(owner: QObject) -> None:
        setattr(TimeKeeper, 'hertz', {
            30: QTimer(owner, interval=33),
            60: QTimer(owner, interval=16)
        })
        setattr(TimeKeeper, 'seconds', {
            1:  QTimer(owner, interval=1000),
            5:  QTimer(owner, interval=5000),
            10: QTimer(owner, interval=10000),
            30: QTimer(owner, interval=30000),
            60: QTimer(owner, interval=60000)
        })
        
        setattr(TimeKeeper, '_dT_timer', QElapsedTimer())
        
        
    @staticmethod
    def update_frame_delta() -> None:
        TimeKeeper._frame_delta = TimeKeeper._dT_timer.restart() / 1000.0
        
    @staticmethod
    def get_frame_delta() -> float: return TimeKeeper._frame_delta
    
    @staticmethod
    def start() -> None:
        for timer in TimeKeeper.hertz.values():   timer.start()
        for timer in TimeKeeper.seconds.values(): timer.start()
        TimeKeeper._dT_timer.start(); TimeKeeper.update_frame_delta()
        
    @staticmethod
    def dT() -> float: return TimeKeeper.get_frame_delta()

