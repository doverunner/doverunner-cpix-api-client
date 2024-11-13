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
    widevine_hls_signaling_data_master: str = ""
    widevine_hls_signaling_data_media: str = ""
    playready_pssh: str = ""
    playready_pssh_payload: str = ""
    playready_smoothstreaming_data: str = ""
    playready_hls_signaling_data_master: str = ""
    playready_hls_signaling_data_media: str = ""
    fairplay_hls_key_uri: str = ""
    fairplay_hls_signaling_data_master: str = ""
    fairplay_hls_signaling_data_media: str = ""
    ncg_cek: str = ""
    ncg_hls_key_uri: str = ""
    wiseplay_pssh: str = ""
    wiseplay_pssh_payload: str = ""
    wiseplay_hls_signaling_data_master: str = ""
    wiseplay_hls_signaling_data_media: str = ""


@dataclass
class ContentPackagingInfo:
    content_id: str = ""
    multidrm_infos: list[MultiDrmInfo] = field(default_factory=list)
