from enum import Flag, auto


class EncryptionScheme(Flag):
    CENC = auto()
    CBC1 = auto()
    CENS = auto()
    CBCS = auto()
