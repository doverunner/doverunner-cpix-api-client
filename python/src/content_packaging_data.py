from dataclasses import dataclass, field


@dataclass
class MultiDrmInfo:
    track_type: str = ""
    key: str = ""
    key_id: str = ""
    iv: str = ""
    period_index: str = ""
    widevine_pssh: str = ""
    widevine_pssh_payload: str = ""
    playready_pssh: str = ""
    playready_pssh_payload: str = ""
    fairplay_hls_signaling_data: str = ""
    fairplay_hls_key_uri: str = ""
    ncg_cek: str = ""
    ncg_hls_key_uri: str = ""


@dataclass
class ContentPackagingInfo:
    content_id: str = ""
    multidrm_infos: list[MultiDrmInfo] = field(default_factory=list)
