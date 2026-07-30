from typing import Self
from dataclasses import dataclass

from PySide6 import QtWidgets as W
from PySide6.QtGui import QColor, QPen, QPainter, QMouseEvent
from PySide6.QtCore import Qt, Slot, QPointF, QEvent, Signal, Slot

from nectar_render import SceneInterface, Transform, Vector2, Vector3, Color
from nectar_render.gui.bridge   import Bridge
from nectar_render.gui.registry import WidgetRegistry
from nectar_render.gui          import utils
from nectar_render.gui.widgets.vector import VectorWidget

AXES = [
    (Vector3(1, 0, 0), QColor(220, 60, 60)),
    (Vector3(0, 1, 0), QColor(60, 200, 80)),
    (Vector3(0, 0, 1), QColor(70, 120, 230)),
]

HANDLE_SCREEN_LENGTH = 30.0
HIT_TOLERANCE = 8.0

@dataclass
class DragState:
    axis_index: int
    start_mouse: QPointF
    start_position: Vector3
    world_per_pixel: Vector3


class TransformGnomon(W.QWidget):
    transform_updated = Signal(Transform)
    
    def __init__(self: Self) -> None:
        super().__init__(WidgetRegistry.viewport)
        WidgetRegistry.register_transform_gnomon(self)
        WA = Qt.WidgetAttribute
        self.setAttribute(WA.WA_TransparentForMouseEvents, True)
        self.setAttribute(WA.WA_NoSystemBackground)
        self.setAttribute(WA.WA_TranslucentBackground)

        self._drag: DragState | None = None
        self._hover_axis: int | None = None

        self.resize(WidgetRegistry.viewport.size())

    def _project(self: Self, world_point: Vector3) -> QPointF | None:
        result = Bridge.camera.project_to_screen(world_point)
        if result is None:
            return None
        pixmap = self.parentWidget().pixmap()
        if pixmap is None or pixmap.isNull():
            return None

        label_w, label_h = self.width(), self.height()
        pix_w, pix_h = pixmap.width(), pixmap.height()
        scale = min(label_w / pix_w, label_h / pix_h)
        displayed_w, displayed_h = pix_w * scale, pix_h * scale
        offset_x = (label_w - displayed_w) / 2.0
        offset_y = (label_h - displayed_h) / 2.0

        return QPointF(
            result.x() * displayed_w + offset_x,
            result.y() * displayed_h + offset_y,
        )

    def _handle_positions(
        self: Self
    ) -> list[tuple[QPointF, QPointF, QColor]] | None:
        if not Bridge.scene_interface.is_enabled():
            return None

        xform = Bridge.scene_interface.get_transform()
        origin = xform.position()

        cam_pos = Bridge.camera.parameters().position
        distance = (origin - cam_pos).length()
        world_length = distance * (HANDLE_SCREEN_LENGTH / self.height()) * 2.0

        origin_screen = self._project(origin)
        if origin_screen is None:
            return None

        handles = []
        for direction, color in AXES:
            tip_world = origin + direction * world_length
            tip_screen = self._project(tip_world)
            if tip_screen is None:
                continue
            handles.append((origin_screen, tip_screen, color))
        return handles

    def paintEvent(self: Self, event: QEvent) -> None:
        handles = self._handle_positions()
        if not handles:
            return

        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)

        for i, (origin, tip, color) in enumerate(handles):
            is_active = (self._drag is not None and i == self._drag.axis_index)
            is_hovered = (self._drag is None and i == self._hover_axis)

            if is_active:    draw_color = QColor(255, 255, 255); width = 6
            elif is_hovered: draw_color = color.lighter(150);    width = 6
            else:            draw_color = color;                 width = 4

            p.setPen(QPen(draw_color, width))
            p.drawLine(origin, tip)

    @staticmethod
    def _dist_to_segment(pt: QPointF, a: QPointF, b: QPointF) -> float:
        ab = b - a
        length_sq = ab.x() ** 2 + ab.y() ** 2
        if length_sq < 1e-6:
            return (pt - a).manhattanLength()
        t = max(0.0, min(1.0, QPointF.dotProduct(pt - a, ab) / length_sq))
        closest = a + ab * t
        d = pt - closest
        return (d.x() ** 2 + d.y() ** 2) ** 0.5

    def _hit_test(self: Self, pos: QPointF) -> int | None:
        handles = self._handle_positions()
        if not handles:
            return None
        for i, (origin, tip, _) in enumerate(handles):
            if self._dist_to_segment(pos, origin, tip) <= HIT_TOLERANCE:
                return i
        return None

    @property
    def is_dragging(self: Self) -> bool:
        return self._drag is not None

    def try_begin_drag(self, pos: QPointF) -> bool:
        axis = self._hit_test(pos)
        if axis is None:
            return False

        xform = Bridge.scene_interface.get_transform()
        direction, _ = AXES[axis]

        origin_screen = self._project(xform.position())
        tip_screen = self._project(xform.position() + direction)
        if origin_screen is None or tip_screen is None:
            return False

        screen_delta = tip_screen - origin_screen
        screen_len_sq = screen_delta.x() ** 2 + screen_delta.y() ** 2
        if screen_len_sq < 1e-6:
            return False

        world_per_pixel = direction * (1.0 / (screen_len_sq ** 0.5))

        self._drag = DragState(
            axis_index=axis,
            start_mouse=pos,
            start_position=xform.position(),
            world_per_pixel=world_per_pixel,
        )
        return True

    def update_hover(self: Self, pos: QPointF) -> None:
        self._hover_axis = self._hit_test(pos)
        self.update()

    def continue_drag(self: Self, pos: QPointF) -> None:
        if self._drag is None:
            return

        direction, _ = AXES[self._drag.axis_index]
        mouse_delta = pos - self._drag.start_mouse

        origin_screen = self._project(self._drag.start_position)
        tip_screen = self._project(self._drag.start_position + direction)
        if origin_screen is None or tip_screen is None:
            return

        screen_dir = tip_screen - origin_screen
        screen_len = (screen_dir.x() ** 2 + screen_dir.y() ** 2) ** 0.5
        if screen_len < 1e-6:
            return

        signed_pixels = (
            mouse_delta.x() * screen_dir.x() + mouse_delta.y() * screen_dir.y()
        ) / screen_len

        world_distance = signed_pixels / screen_len

        new_position = self._drag.start_position + direction * world_distance
        
        
        xform = Bridge.scene_interface.get_transform()
        new_xform = Transform(
            new_position, xform.rotation().forward(), xform.scale()
        )

        Bridge.scene_interface.set_transform(new_xform)
        self.transform_updated.emit(new_xform)
        self.update()

    def end_drag(self: Self) -> None:
        if self._drag is None: return
        self._drag = None
        self.update()
        
    def __del__(self: Self) -> None:
        WidgetRegistry.unregister(self)
        
        
        

class XformController(W.QGroupBox):
    def __init__(self: Self) -> None:
        super().__init__()
        self.setTitle('Transform')
        
        layout = W.QVBoxLayout()
        layout.setContentsMargins(3, 3, 3, 3)
        self.setLayout(layout)

        self.xform = Bridge.scene_interface.get_transform()    
        self.scale    = VectorWidget(values=self.xform.scale().as_array())
        self.position = VectorWidget(values=self.xform.position().as_array())
        self.rotation = VectorWidget(
            values=self.xform.rotation().forward().as_array()
        )
        
        layout = W.QFormLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addRow('Translate', self.position)
        layout.addRow('Rotate',    self.rotation)
        layout.addRow('Scale',     self.scale)
        
        frame = W.QFrame()
        frame.setLayout(layout)
        self.layout().addWidget(frame)
        
        update_btn = W.QPushButton('Update')
        update_btn.clicked.connect(self.update)
        self.layout().addWidget(update_btn)
        
        test_btn = W.QPushButton('test')
        test_btn.clicked.connect(lambda : None)
        self.layout().addWidget(test_btn)
        
        
        self.xform_gnomon = TransformGnomon()
        self.xform_gnomon.transform_updated.connect(self._update_from_gnomon)
        self.xform_gnomon.show()
        
        
    @Slot()
    def _update_from_gnomon(self: Self, new_xform: Transform) -> None:
        self.position.set_from_vector(new_xform.position())
        self.rotation.set_from_vector(new_xform.rotation().forward())
        self.scale.set_from_vector(new_xform.scale())
        
        
    # def object_position_screen_space(self: Self) -> Vector2 | None:
    #     return Bridge.camera.project_to_screen(self.xform.position())
        
     
    # def object_position_pixel_space(self: Self) -> QPointF | None:
    #     cam_ndc_pos = self.object_position_screen_space()
    #     if cam_ndc_pos is None: return cam_ndc_pos
        
    #     viewport_size = Bridge.viewport.size()
    #     pixmap_size = Bridge.viewport.pixmap().size()
        
    #     label_w, label_h = viewport_size.width(), viewport_size.height()
    #     pix_w, pix_h = pixmap_size.width(), pixmap_size.height()

    #     scale = min(label_w / pix_w, label_h / pix_h)
    #     displayed_w = pix_w * scale
    #     displayed_h = pix_h * scale

    #     offset_x = (label_w - displayed_w) / 2.0
    #     offset_y = (label_h - displayed_h) / 2.0

    #     pixel_x = int(cam_ndc_pos.x() * displayed_w + offset_x)
    #     pixel_y = int(cam_ndc_pos.y() * displayed_h + offset_y)
    #     return QPointF(pixel_x, pixel_y)
     
    
    def update(self: Self) -> None:
        xform = Transform(
            self.position.as_vector3(),
            self.rotation.as_vector3(),
            self.scale.as_vector3()
        )
        Bridge.scene_interface.set_transform(xform)


