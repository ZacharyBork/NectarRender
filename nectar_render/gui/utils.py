from pathlib import Path
from dataclasses import dataclass, field

import PySide6.QtWidgets as W
from PySide6.QtCore import QObject, QSize, QTimer, QElapsedTimer
from PySide6.QtGui  import QIcon

def load_icon(filename: str) -> QIcon:
    root = Path(__file__).parent.resolve()
    file = root / f'resource/icons/{filename}.png'
    return QIcon(file.as_posix())

def set_button_icon(
    button:   W.QPushButton,
    filename: str, 
    size:     tuple[int, int] = (16, 16)
) -> None:
    button.setIcon(load_icon(filename))
    button.setIconSize(QSize(*size))

@dataclass
class TimeKeeper:
    hertz: dict[int, QTimer] = field(default_factory=lambda : {
        5: None, 10: None, 30: None, 60: None
    })
    seconds: dict[int, QTimer] = field(default_factory=lambda : {
        1: None, 5: None, 10: None, 30: None, 60: None
    })
    
    _dT_timer: QElapsedTimer = None
    _frame_delta: float = 0.0
    
    _owned:  bool = False
    _active: bool = False
    
    @staticmethod
    def set_owner(owner: QObject) -> None:
        setattr(TimeKeeper, 'hertz', {
            5:  QTimer(owner, interval=200),
            10: QTimer(owner, interval=100),
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
        setattr(TimeKeeper, '_owned', True)
        
    @staticmethod
    def start() -> None:
        for timer in TimeKeeper.hertz.values():   timer.start()
        for timer in TimeKeeper.seconds.values(): timer.start()
        TimeKeeper._dT_timer.start(); TimeKeeper.update_frame_delta()
        setattr(TimeKeeper, '_active', True)
        
    @staticmethod
    def stop() -> None:
        for timer in TimeKeeper.hertz.values():   timer.stop()
        for timer in TimeKeeper.seconds.values(): timer.stop()
        TimeKeeper._dT_timer.stop(); TimeKeeper._frame_delta = 0.0
        setattr(TimeKeeper, '_active', False)
        
    @staticmethod
    def is_active() -> bool: return TimeKeeper._active
        
    @staticmethod
    def update_frame_delta() -> None:
        TimeKeeper._frame_delta = TimeKeeper._dT_timer.restart() / 1000.0
        
    @staticmethod
    def get_frame_delta() -> float: return TimeKeeper._frame_delta
    
    @staticmethod
    def dT() -> float: return TimeKeeper.get_frame_delta()
    
    

