import sys
from typing  import Self
from pathlib import Path

from PySide6 import QtWidgets as W
from PySide6.QtCore    import Qt, QFile, QObject, Slot
from PySide6.QtGui     import QAction
from PySide6.QtUiTools import QUiLoader

from nectar_render import Camera, EnginePollResponse
from nectar_render.gui          import utils
from nectar_render.gui.utils    import TimeKeeper
from nectar_render.gui.bridge   import Bridge
from nectar_render.gui.registry import WidgetRegistry
from nectar_render.gui.widgets import (
    ViewportWidget, ColorCorrection, ProgressBar, Profiler, MenuBar,
    CollapsibleMenu, SpinboxSlider
)

from nectar_render.gui.settings_groups import SkylightSettings

from nectar_render.scenes.cornell_box import CornellBox

###############################################################################
# INTERFACE CLASS
###############################################################################

from nectar_render.python import (
    Hittable, Vector3, Color, Material, Scene, Skylight
)
asset_root = Path(__file__).parent.parent.parent / 'tmp/assets'
test_scene = Scene(
    skylight  = Skylight.hdri(
        asset_root.resolve().as_posix() + '/hdri/brown_photostudio.hdr'
    ),
    lights    = [
        # Hittable.OBJECT_LIGHT(
        #     Hittable.QUAD(
        #         Vector3(1.0, 1, 0.0),
        #         Vector3(0.0, 180.0, -45.0),
        #         Vector3(1),
        #         Material.LAMBERTIAN(Color.white())
        #     ),
        #     15.0, Color(1.0, 0.4, 0.2)
        # )
    ],
    hittables = [
        Hittable.QUAD( # Bottom
            Vector3(0.0, -0.5, 0.0),
            Vector3(0.0, 0.0, 0.0),
            Vector3(100.0),
            Material.PBR(
                asset_root.resolve().as_posix() + '/tile.jpeg',
                0.15,
                0.2
            )
        ),
        # Hittable.QUAD( # Top
        #     Vector3(0.0, 0.5, 0.0),
        #     Vector3(0.0, 180.0, 0.0),
        #     Vector3(1.0),
        #     Material.LAMBERTIAN(Color.white())
        # ),
        # Hittable.QUAD( # Right
        #     Vector3(0.5, 0.0, 0.0),
        #     Vector3(0.0, 0.0, 90.0),
        #     Vector3(1.0),
        #     Material.LAMBERTIAN(Color.red())
        # ),
        # Hittable.QUAD( # Left
        #     Vector3(-0.5, 0.0, 0.0),
        #     Vector3(0.0, 0.0, -90.0),
        #     Vector3(1.0),
        #     Material.LAMBERTIAN(Color.green())
        # ),
        Hittable.SPHERE( # Back
            Vector3(0.5, -0.3, 0.0),
            0.2, Material.PBR(Color.white(), 0.1, 1.0)
        ),
        Hittable.SPHERE( # Back
            Vector3(0.7, -0.3, 0.5),
            0.2, Material.PBR(Color.yellow())
        ),
        Hittable.SPHERE( # Back
            Vector3(0.8, -0.3, 1.1),
            0.2, Material.DIELECTRIC(1.5)
        ),
        
        Hittable.MESH(
            asset_root.resolve().as_posix() + '/happy.obj',
            Vector3(0.0, -0.8, 0.0),
            Vector3(0.0, 45.0, 0.0),
            Vector3(6.0, 6.0, 6.0),
            Material.PBR(
                albedo    = Color(1.0, 0.6, 0.15),
                roughness = 0.4,
                metallic  = 1.0 
            )
        )
        
    ]
)

class Interface(QObject):    
    def __init__(self: Self) -> None:
        super().__init__()
        
        WidgetRegistry.register_interface(self)
        
        self.viewport:  ViewportWidget = None
        self.progress_bar: ProgressBar = None
        self.profiler:        Profiler = None
        self.color_correction:  ColorCorrection = None
        
        self.camera = Camera(
            resolution   = (1024, 1028),
            position     = (0.0, 0.0, 2.0),
            rotation     = (0.0, 0.0, 0.0),
            num_samples  = 1024,
            focal_length = 3.0
        )
        self.scene = test_scene
        self.max_depth: int = 6
        self.seed:      int = 42

        Bridge.init(self.camera, self.max_depth, self.seed)
        Bridge.set_scene(self.scene)
        Bridge.signals.frame_finished.connect(self._on_frame_finished)
        Bridge.signals.render_finished.connect(self._on_render_finished)
        Bridge.ENGINE.poll_updates = self._engine_poll
        
        TimeKeeper.set_owner(self)
        
#### ENGINE UTILITIES #########################################################

    def _engine_poll(self: Self) -> None:
        response = EnginePollResponse()
        TimeKeeper.update_frame_delta()
        
        self.viewport.camera_controller.update_transforms(TimeKeeper.dT())
        if self.viewport.camera_controller.poll_updates():
            response.camera_params = self.viewport.camera_controller.params
            response.should_update_camera = True
            self.viewport.gnomon.update_rotation()
        
        return response
                
#### ENGINE HOOKS #############################################################
    
    @Slot(int)
    def _on_frame_finished(self: Self, frame_idx: int) -> None:
        self.progress_bar.update(frame_idx, Bridge.n_samples)

    @Slot()
    def _on_render_finished(self: Self) -> None:
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
        
#### CALLBACKS ################################################################

    def toggle_groupbox_widgets(self: Self, name: str) -> None:
        box = self.find(W.QGroupBox, name)
        widgets = [i for i in box.children() if hasattr(i, 'setVisible')]
        
        for widget in widgets:
            visible = not widget.isVisible()
            widget.setVisible(visible)
            if visible: box.setStyleSheet('padding: 3px;')
            else: box.setStyleSheet('padding: -5px;')

    def play_button(self: Self) -> None:
        Bridge.requests.start()
        self.find(W.QPushButton, 'play').setEnabled(False)
        self.find(W.QPushButton, 'pause').setEnabled(True)
        self.find(W.QPushButton, 'stop').setEnabled(True)
        
    def pause_button(self: Self) -> None:
        # Bridge.ENGINE.request_pause()
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(True)
        
    def stop_button(self: Self) -> None:
        Bridge.requests.stop()
        self.find(W.QPushButton, 'play').setEnabled(True)
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
        
    def refresh(self: Self) -> None:
        if not Bridge.is_rendering: return
        Bridge.requests.restart()

    def set_n_samples(self: Self) -> None:
        pass
        # value = self.find(W.QSpinBox, 'n_samples').value()
        # Bridge.queue_function(
        #     lambda : Bridge.ENGINE.set_n_samples(value)
        # )

    def set_max_depth(self: Self) -> None:
        pass
        # value = self.find(W.QSpinBox, 'max_depth').value()
        # Bridge.queue_function(
            # lambda : Bridge.ENGINE.set_max_depth(value)
        # )
            
    def _select_render_pass(self: Self, index: int) -> None:
        # TODO: Implement render pass switching
        pass

#### INITIALIZATION ###########################################################

    def _init_mainwidget(self: Self) -> W.QWidget:
        path = Path(__file__).parent.resolve() / 'resource/mainwidget.ui'
        if not path.exists():
            msg = f'Unable to locate UI file: {path.resolve().as_posix()}'
            raise FileNotFoundError(msg)
        
        file = QFile(path.as_posix())
        file.open(QFile.ReadOnly)
        widget = QUiLoader().load(file)
        file.close()
        return widget

    def _set_stylesheet(self: Self) -> None:
        path = Path(__file__).parent.resolve() / 'resource/stylesheet.qss'
        if not path.exists():
            raise FileNotFoundError(
                f'Unable to locate stylesheet: {path.as_posix()}'
            )
        with open(path.as_posix(), 'r') as file:
            self.app.setStyleSheet(file.read())

    def _build_viewport(self: Self) -> None:
        self.viewport = ViewportWidget(
            self.find(W.QTabWidget, 'settings_tabs')
        )
        
        frame = self.find(W.QFrame, 'viewport_frame')
        frame.layout().addWidget(self.viewport)

    def _init_callbacks(self: Self) -> None:        
        connect_button = lambda name, fn : (
            self.find(W.QPushButton, name).clicked.connect(fn)
        )
        
        connect_button('play',    self.play_button)
        connect_button('pause',   self.pause_button)
        connect_button('stop',    self.stop_button)
        connect_button('refresh', self.refresh)
        connect_button('save_render', self.viewport.save_image)
        
        connect_spinbox = lambda name, fn : (
            self.find(W.QSpinBox, name).editingFinished.connect(fn)
        )
        
        connect_spinbox('n_samples', self.set_n_samples)
        connect_spinbox('max_depth', self.set_max_depth)
        
        connect_groupbox = lambda name : (
            self.find(W.QGroupBox, name).clicked.connect(
                lambda : self.toggle_groupbox_widgets(name)
            )
        )

        connect_groupbox('general_settings')
        connect_groupbox('camera_settings')
        connect_groupbox('color_correction_settings')
        
    def _build_control_bar(self: Self) -> None:        
        icon_paths = {
            'play':        ('play',    (16, 16)),
            'pause':       ('pause',   (16, 16)),
            'stop':        ('stop',    (16, 16)),
            'refresh':     ('refresh', (16, 16)),
            'save_render': ('save',    (16, 16))
        }
        
        for name, (file, size) in icon_paths.items():
            btn = self.find(W.QPushButton, name)
            utils.set_button_icon(btn, file, size)
            
        self.find(W.QPushButton, 'pause').setEnabled(False)
        self.find(W.QPushButton, 'stop').setEnabled(False)
        
        render_pass = self.find(W.QComboBox, 'render_pass')
        render_pass.currentIndexChanged.connect(self._select_render_pass)
        render_pass.addItem('Beauty')
        render_pass.addItem('Normal (WS)')
        
    def _build_profiler(self: Self) -> None:
        tab = self.find(W.QWidget, 'profiler_tab')
        self.profiler = Profiler(tab)    
        
    def _init_menu_bar(self: Self) -> None:
        self._menubar = MenuBar(self)

#### SETTINGS TAB #############################################################


    def build_settings_tab(self: Self) -> None:
        frame = self.find(W.QFrame, 'settings_frame')
        frame.layout().addWidget(SkylightSettings())
        
        
#### ENTRYPOINT ###############################################################

    def run(self: Self) -> None:
        self.app = W.QApplication(sys.argv)
        self._set_stylesheet()

        self.mainwidget = self._init_mainwidget()
        self.mainwidget.setWindowTitle('NectarRender')
        self.find = self.mainwidget.findChild

        self._build_viewport()
        self._build_control_bar()
        self._build_profiler()
        self._init_callbacks()
        self._init_menu_bar()
        
        self.build_settings_tab()
        
        self.progress_bar = ProgressBar(
            self.find(W.QFrame, 'progress_frame').layout()
        )
        self.color_correction = ColorCorrection(
            self.find(W.QGroupBox, 'color_correction_settings')
        )
        
        TimeKeeper.start()
        Bridge.thread.start()
        self.mainwidget.show()
        
        sys.exit(self.app.exec())



