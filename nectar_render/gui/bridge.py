from __future__ import annotations
from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from nectar_render.gui.widgets import ViewportWidget

import _pathtracer
root = _pathtracer.engine

import threading
from typing          import Self, TypeAlias
from collections.abc import Callable

import ctypes
import numpy as np
from PySide6.QtCore import QObject, Signal, Slot

from nectar_render.python import (
    Camera, Scene, SceneInterface, TransferStream, StreamConfig
)

Engine: TypeAlias = root.RenderEngine

###############################################################################
# SIGNAL INTERFACE
###############################################################################

class SignalInterface(QObject):
    render_started  = Signal()
    frame_finished  = Signal(int)
    render_finished = Signal()
    
    stopped   = Signal()
    restarted = Signal()
    reset     = Signal()
    shutdown  = Signal()
        
    def __init__(self: Self, engine: root.RenderEngine) -> None:
        super().__init__()
        engine.on_render_started  = self.render_started.emit
        engine.on_render_finished = self.render_finished.emit
        engine.on_frame_finished  = (
            lambda frame_idx : self.frame_finished.emit(frame_idx)
        )
        
        engine.on_stopped   = self.stopped.emit
        engine.on_restarted = self.restarted.emit        
        engine.on_reset     = self.reset.emit
        engine.on_shutdown  = self.shutdown.emit
    
###############################################################################
# THREAD MANAGER
###############################################################################
        
class ThreadWorker:
    THREAD: threading.Thread = None
    ENGINE: Engine = None
    
    def __init__(self: Self, engine: Engine) -> None:
        setattr(ThreadWorker, 'ENGINE', engine)
        
    @property
    def is_alive(self: Self) -> bool: 
        return self.THREAD.is_alive() if self.THREAD is not None else False
    
    def stop(self: Self) -> None:
        if self.THREAD is not None: self.THREAD.join()
        self.THREAD = None
        
    def start(self: Self) -> None:
        if self.is_alive: return
        self.THREAD = threading.Thread(target=self.ENGINE.idle, daemon=True)
        self.THREAD.start()
        
    def __del__(self: Self) -> None: 
        if not self.is_alive: return
        self.ENGINE.request_shutdown()
        self.stop()

###############################################################################
# GLOBAL RENDER BRIDGE ACCESS
###############################################################################

class BridgeMeta(type):
    ENGINE: Engine = None
    
    _stream_config: StreamConfig = None
    _thread:        ThreadWorker = None
    _signals:    SignalInterface = None
    
    @staticmethod
    def init(camera: Camera, ray_depth: int, seed: int) -> None:
        setattr(BridgeMeta, 'ENGINE', Engine(camera, ray_depth, seed))
        setattr(BridgeMeta, '_stream_config', StreamConfig())
        setattr(BridgeMeta, '_thread',  ThreadWorker(BridgeMeta.ENGINE))
        setattr(BridgeMeta, '_signals', SignalInterface(BridgeMeta.ENGINE))

    @property
    def thread(cls: type[Self]) -> ThreadWorker: 
        return BridgeMeta._thread
    
    @property
    def signals(cls: type[Self]) -> SignalInterface: 
        return BridgeMeta._signals
    
    @property
    def requests(cls: type[Self]) -> root.EngineRequests: 
        return BridgeMeta.ENGINE.requests()
    
    @property
    def scene(cls: type[Self]) -> Scene:
        return BridgeMeta.ENGINE.scene()

    @property
    def scene_interface(cls: type[Self]) -> SceneInterface:
        return BridgeMeta.ENGINE.get_scene_interface()
    
    @property
    def stream(cls: type[Self]) -> TransferStream:
        return BridgeMeta.ENGINE.stream()
    
    @property
    def stream_config(cls: type[Self]) -> StreamConfig:
        return BridgeMeta._stream_config
    
    @property
    def state(cls: type[Self]) -> root.EngineState: 
        return BridgeMeta.ENGINE.get_state()
    
    @property
    def is_idle(cls: type[Self]) -> bool: 
        return BridgeMeta.ENGINE.is_idle()
    
    @property
    def is_rendering(cls: type[Self]) -> bool: 
        return BridgeMeta.ENGINE.is_rendering()
    
    @property
    def n_samples(cls: type[Self]) -> int:
        return BridgeMeta.ENGINE.n_samples()
    
    @property
    def camera(cls: type[Self]) -> Camera: 
        return BridgeMeta.ENGINE.camera()


class Bridge(metaclass=BridgeMeta):
    
    @staticmethod
    def init(
        camera:    Camera = Camera(),
        ray_depth: int = 8,
        seed:      int | None = None
    ) -> None:
        BridgeMeta.init(camera, ray_depth, seed)
        Bridge.set_scene = Bridge.ENGINE.set_scene   
        
    @staticmethod
    def start_if_idle() -> None:
        if not Bridge.is_idle: return
        Bridge.requests.start()
        
    @staticmethod
    def update_stream_config() -> None:
        Bridge.stream.update_config(Bridge.stream_config)
        
    @staticmethod
    def readback_stream() -> np.ndarray:
        ptr = Bridge.stream.readback()
        arr = np.ctypeslib.as_array(
            (ctypes.c_uint8 * Bridge.stream.n_elements()).from_address(ptr)
        )
        arr = arr.reshape(*Bridge.stream.shape()).transpose(1, 2, 0)
        return np.ascontiguousarray(arr)
    
    @staticmethod
    def state_as_string() -> str:
        match Bridge.state:
            case root.EngineState.IDLE:      return 'Idle'
            case root.EngineState.RENDERING: return 'Rendering'



