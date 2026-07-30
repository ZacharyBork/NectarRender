from __future__ import annotations
from typing     import TYPE_CHECKING, Self, Any
if TYPE_CHECKING:
    from nectar_render.gui.interface import Interface
    from nectar_render.gui.widgets import ViewportWidget
    from nectar_render.gui.widgets.xform_controller import TransformGnomon

###############################################################################
# WIDGET REGISTRY
###############################################################################

class WidgetRegistryMeta(type):
    
#### MAIN INTERFACE ###########################################################
    
    _interface: Interface = None

    @staticmethod
    def register_interface(instance: Interface) -> None:
        WidgetRegistryMeta._interface = instance

    @property
    def interface(cls: type[Self]) -> Interface: 
        return WidgetRegistryMeta._interface
    
#### VIEWPORT WIDGET ##########################################################
    
    _viewport: ViewportWidget = None

    @staticmethod
    def register_viewport(instance: ViewportWidget) -> None:
        WidgetRegistryMeta._viewport = instance

    @property
    def viewport(cls: type[Self]) -> ViewportWidget: 
        return WidgetRegistryMeta._viewport
    
#### TRANSFORM GNOMON #########################################################
    
    _transform_gnomon: TransformGnomon | None = None
    
    @staticmethod
    def register_transform_gnomon(instance: TransformGnomon) -> None:
        WidgetRegistryMeta._transform_gnomon = instance
    
    @property
    def transform_gnomon(cls: type[Self]) -> TransformGnomon | None: 
        return WidgetRegistryMeta._transform_gnomon


class WidgetRegistry(metaclass=WidgetRegistryMeta):
    
    @staticmethod
    def unregister(instance: Any) -> bool:
        for key, value in WidgetRegistryMeta.__dict__.items():
            if instance == value:
                WidgetRegistryMeta.__dict__[key] = None
                return True
        return False
                





