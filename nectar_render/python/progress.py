import tqdm
from typing import Self
from dataclasses import dataclass

PROGRESS_BARS: dict[str, "ProgressBar"] = {}

@dataclass
class ProgressBar:
    total: int
    name:  str
    pbar:  tqdm.tqdm = None
    
    FORMAT = (
        '{desc} {percentage:3.0f}% ▐{bar:30}▌ '
        '{n_fmt}/{total_fmt} samples '
        '[{elapsed}<{remaining}, {rate_fmt}]'
    )
    
    def __post_init__(self: Self) -> None:
        self.pbar = tqdm.tqdm(
            total      = self.total,
            desc       = 'Rendering',
            disable    = False,
            unit       = '',
            colour     = '#5CF24E',
            bar_format = self.FORMAT,
            ascii      = ' ▏▎▍▌▋▊▉█'
        )
    
    def update(self: Self, current: int) -> None:
        global PROGRESS_BARS
        self.pbar.update(1)
        
        if current >= self.total:
            self.pbar.close()
            self.pbar = None
            PROGRESS_BARS.pop(self.name)
    
def make_progress_bar(key: str, total: int) -> None:
    global PROGRESS_BARS
    if key in PROGRESS_BARS:
        raise KeyError(f'Key [{key}] already exists in progress bar cache.')
    PROGRESS_BARS[key] = ProgressBar(total, key)
    
def get_progress_bar(key: str) -> ProgressBar: 
    global PROGRESS_BARS
    return PROGRESS_BARS[key]
