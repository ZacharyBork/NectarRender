from __future__ import annotations

import _pathtracer
root = _pathtracer.engine

import threading
from typing          import Self
from collections.abc import Callable

from PySide6.QtCore import QObject, Signal, Slot

from nectar_render.python import (
    RenderEngine, Camera, Scene, SceneInterface, TransferStream, StreamConfig
)

###############################################################################
# SIGNAL INTERFACE
###############################################################################

class SignalInterface(QObject):
    render_started  = Signal()
    render_finished = Signal()
    frame_finished  = Signal(int)
    stopped         = Signal()
    reset           = Signal()
    
    def __init__(self: Self, engine: root.RenderEngine) -> None:
        super().__init__()
        engine.on_reset          = self.reset.emit
        engine.on_stopped        = self.stopped.emit
        engine.on_render_started = self.render_started.emit
        engine.on_frame_finished = (
            lambda frame_idx : self.frame_finished.emit(frame_idx)
        )
    
###############################################################################
# RENDER BRIDGE CLASS
###############################################################################
        
class RenderBridge(RenderEngine):
    THREAD: threading.Thread = None
    SCENE:  Scene = None
    
    def __init__(
        self:      Self, 
        camera:    Camera,
        max_depth: int = 8,
        seed:      int | None = None
    ) -> None:
        super().__init__(camera, max_depth, seed, silent=True)
        self.signals = SignalInterface(self.ENGINE)
                
    @property
    def is_alive(self: Self) -> bool: 
        return self.THREAD.is_alive() if self.THREAD is not None else False
    
    def stop_thread(self: Self) -> None:
        if self.THREAD is not None: self.THREAD.join()
        self.THREAD = None
        
    def start_thread(self: Self) -> None:
        self.THREAD = threading.Thread(target=self.render, daemon=True)
        self.THREAD.start()

###############################################################################
# GLOBAL RENDER BRIDGE ACCESS
###############################################################################

class BridgeMeta(type):
    _instance:      RenderBridge = None
    _stream_config: StreamConfig = None

    @staticmethod
    def set_instance(bridge_instance: RenderBridge) -> None:
        setattr(BridgeMeta, '_instance', bridge_instance) 
        setattr(BridgeMeta, '_stream_config', StreamConfig())

    @property
    def instance(self: Self) -> RenderBridge: 
        if BridgeMeta._instance is not None: return BridgeMeta._instance
        raise RuntimeError(
            f'Bridge.instance accessed on Bridge object prior to bridge being '
            f'set. Please call Bridge.set_instance() first to set the global '
            f'RenderBridge instance.'
        )
        
    @property
    def scene_interface(self: Self) -> SceneInterface:
        return BridgeMeta._instance.ENGINE.get_scene_interface()
    
    @property
    def transfer_stream(self: Self) -> TransferStream:
        return BridgeMeta._instance.ENGINE.stream()
    
    @property
    def stream_config(self: Self) -> StreamConfig:
        return BridgeMeta._stream_config

class Bridge(metaclass=BridgeMeta):
    
    @staticmethod
    def set_instance(bridge_instance: RenderBridge) -> None:
        BridgeMeta.set_instance(bridge_instance)
        Bridge.instance.signals.stopped.connect(Bridge._restart_render)
        
    @staticmethod
    def queue_function(
        func:        Callable[[], None], 
        rebuild_bvh: bool = False,
        immediate:   bool = True
    ) -> None:
        Bridge.instance.queue_function(func, rebuild_bvh, immediate)
        Bridge.instance.request_stop()
        
    @Slot()
    def _restart_render() -> None:
        Bridge.instance.stop_thread()
        Bridge.instance.reset()
        Bridge.instance.start_thread()
        
    @staticmethod
    def update_stream_config() -> None:
        Bridge.transfer_stream.update_config(Bridge.stream_config)
        

