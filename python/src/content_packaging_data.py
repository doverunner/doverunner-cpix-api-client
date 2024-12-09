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
    wiseplay_pssh: str = ""
    wiseplay_pssh_payload: str = ""
    wiseplay_hls_signaling_data_master: str = ""
    wiseplay_hls_signaling_data_media: str = ""
    ncg_cek: str = ""
    ncghls_aes128_key_uri: str = ""
    ncghls_aes128_hls_signaling_data_master: str = ""
    ncghls_aes128_hls_signaling_data_media: str = ""
    ncghls_sampleaes_key_uri: str = ""
    ncghls_sampleaes_hls_signaling_data_master: str = ""
    ncghls_sampleaes_hls_signaling_data_media: str = ""
    aes128_key_uri: str = ""
    aes128_hls_signaling_data_master: str = ""
    aes128_hls_signaling_data_media: str = ""
    sampleaes_key_uri: str = ""
    sampleaes_hls_signaling_data_master: str = ""
    sampleaes_hls_signaling_data_media: str = ""


@dataclass
class ContentPackagingInfo:
    content_id: str = ""
    multidrm_infos: list[MultiDrmInfo] = field(default_factory=list)
