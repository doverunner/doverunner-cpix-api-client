from enum import Flag, auto


class DrmType(Flag):
    WIDEVINE = auto()
    PLAYREADY = auto()
    FAIRPLAY = auto()
    NCG = auto()
    HLS_NCG = auto()
