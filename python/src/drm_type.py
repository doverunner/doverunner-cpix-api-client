from enum import Flag, auto


class DrmType(Flag):
    WIDEVINE = auto()
    PLAYREADY = auto()
    FAIRPLAY = auto()
    WISEPLAY = auto()
    NCG = auto()
    NCGHLS_AES128 = auto()
    NCGHLS_SAMPLEAES = auto()
    AES128 = auto()
    SAMPLEAES = auto()
