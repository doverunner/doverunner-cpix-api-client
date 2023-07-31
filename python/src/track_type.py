from enum import Flag, auto


class TrackType(Flag):
    ALL_TRACKS = auto()
    AUDIO = auto()
    SD = auto()
    HD = auto()
    UHD1 = auto()
    UHD2 = auto()
