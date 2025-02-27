from enum import Flag, auto


class EncryptionScheme(Flag):
    NONE = auto()
    CENC = auto()
    CBC1 = auto()
    CENS = auto()
    CBCS = auto()
