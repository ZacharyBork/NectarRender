from __future__ import annotations

import _pathtracer
root = _pathtracer.engine

import threading
from typing          import Self, Any
from collections.abc import Generator
from contextlib      import ContextDecorator
from dataclasses     import dataclass

from PySide6.QtCore import QObject, Signal

from nectar_render.python import RenderEngine, Scene, Camera

###############################################################################
# RESET CONTEXT MANAGER
###############################################################################

class BridgeReset(ContextDecorator):    
    def __enter__(self: Self) -> Self:
        Bridge.acquire().join_thread()
        return self

    def __exit__(
        self:     Self, 
        *args:    tuple[Any, ...], 
        **kwargs: dict[str, Any]
    ) -> Generator[None, None, None]:
        bridge = Bridge.acquire()
        bridge.request_reset()
        bridge.start()

###############################################################################
# SIGNAL INTERFACE
###############################################################################

class SignalInterface(QObject):
    render_started  = Signal()
    render_finished = Signal()
    frame_finished  = Signal(int)
    canceled        = Signal()
    paused          = Signal()
    reset           = Signal()
    
    def __init__(self: Self, engine: root.RenderEngine) -> None:
        super().__init__()
        engine.on_reset           = self.reset.emit
        engine.on_paused          = self.paused.emit
        engine.on_canceled        = self.canceled.emit
        engine.on_render_started  = self.render_started.emit
        engine.on_render_finished = self.render_finished.emit
        engine.on_frame_finished  = (
            lambda frame_idx : self.frame_finished.emit(frame_idx)
        )
    
###############################################################################
# RENDER BRIDGE CLASS
###############################################################################
        
class RenderBridge(RenderEngine):
    THREAD: threading.Thread = None
    SCENE:  Scene = None
    
    canceled:        Signal = None
    frame_finished:  Signal = None
    render_finished: Signal = None
    
    def __init__(
        self:      Self, 
        scene:     Scene,
        camera:    Camera,
        max_depth: int = 8,
        seed:      int | None = None
    ) -> None:
        super().__init__(camera, max_depth, seed, silent=True)
        self.signals = SignalInterface(self.ENGINE)
        self.update_scene(scene)
        self.build_thread()
        
#### PROPERTIES ###############################################################
        
    @property
    def is_alive(self: Self) -> bool: 
        return self.THREAD.is_alive() if self.THREAD is not None else False
    
    @property
    def camera(self: Self) -> Camera: return self.ENGINE.camera()
        
#### UPDATE UTILITIES #########################################################
        
    def update_scene(self: Self, new_scene: Scene) -> None:
        object.__setattr__(self, 'SCENE', new_scene)
    
#### THREAD UTILITIES #########################################################
    
    def build_thread(self: Self) -> None:
        object.__setattr__(self, 'THREAD', threading.Thread(
            target = self.render, 
            args   = (self.SCENE,), 
            daemon = True
        ))
    
    def join_thread(self: Self) -> None: 
        if self.THREAD is not None: self.THREAD.join()
        self.THREAD = None
        
    def stop(self: Self) -> None:
        self.request_cancel()
        self.join_thread()
        
    def pause(self: Self) -> None:
        self.request_pause()
        self.join_thread()
    
    def start(self: Self) -> None:
        self.build_thread()
        self.THREAD.start()
        
    def reset(self: Self) -> None:
        self.request_reset()
        self.join_thread()

###############################################################################
# GLOBAL RENDER BRIDGE ACCESS
###############################################################################

@dataclass
class Bridge:
    _instance: RenderBridge = None
    
    @staticmethod
    def set_instance(bridge_instance: RenderBridge) -> None:
        setattr(Bridge, '_instance', bridge_instance)
    
    @staticmethod
    def acquire() -> RenderBridge: 
        if Bridge._instance is not None: return Bridge._instance
        raise RuntimeError(
            f'Bridge.acquire() called on BridgeInstance object prior to '
            f'bridge being set. Please call Bridge.set_bridge() first to set '
            f'the global RenderBridge instance.'
        )
        

