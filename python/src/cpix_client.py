import requests
import uuid
import base64
from xml.etree.ElementTree import Element, SubElement, tostring, fromstring
from drm_type import DrmType
from encryption_scheme import EncryptionScheme
from track_type import TrackType
from content_packaging_data import ContentPackagingInfo, MultiDrmInfo

_widevine_system_id = "EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED"
_playready_system_id = "9A04F079-9840-4286-AB92-E65BE0885F95"
_fairplay_system_id = "94CE86FB-07FF-4F43-ADB8-93D2FA968CA2"
_ncg_system_id = "D9E4411A-E886-4909-A380-A77F28D52335"
_hls_ncg_system_id = "48582A1D-1FF4-426E-8CD5-06424FCC578C"


def get_request_data(content_id, drm_type, encryption_scheme, track_type):
    # Setting the key map
    key_map = dict()
    if track_type == TrackType.ALL_TRACKS:
        # Set the track type to "HD" for single-key
        key_map['HD'] = str(uuid.uuid4())
    else:
        if TrackType.AUDIO in track_type:
            key_map['AUDIO'] = str(uuid.uuid4())
        if TrackType.SD in track_type:
            key_map['SD'] = str(uuid.uuid4())
        if TrackType.HD in track_type:
            key_map['HD'] = str(uuid.uuid4())
        if TrackType.UHD1 in track_type:
            key_map['UHD1'] = str(uuid.uuid4())
        if TrackType.UHD2 in track_type:
            key_map['UHD2'] = str(uuid.uuid4())

    req_root = Element("cpix:CPIX")
    req_root.attrib["id"] = content_id
    req_root.attrib["xmlns:cpix"] = "urn:dashif:org:cpix"
    req_root.attrib["xmlns:pskc"] = "urn:ietf:params:xml:ns:keyprov:pskc"
    req_root.attrib["xmlns:speke"] = "urn:aws:amazon:com:speke"
    req_content_key_list = SubElement(req_root, "cpix:ContentKeyList")
    req_content_key_usage_rule_list = SubElement(req_root, "cpix:ContentKeyUsageRuleList")
    req_drm_system_list = SubElement(req_root, "cpix:DRMSystemList")

    for track in key_map:
        SubElement(req_content_key_list, "cpix:ContentKey",
                   {"kid": key_map[track], "commonEncryptionScheme": encryption_scheme.name.lower()})
        SubElement(req_content_key_usage_rule_list, "cpix:ContentKeyUsageRule",
                   {"intendedTrackType": track, "kid": key_map[track]})

        if DrmType.WIDEVINE in drm_type:
            SubElement(req_drm_system_list, "cpix:DRMSystem", {"kid": key_map[track], "systemId": _widevine_system_id})
        if DrmType.PLAYREADY in drm_type:
            SubElement(req_drm_system_list, "cpix:DRMSystem", {"kid": key_map[track], "systemId": _playready_system_id})
        if DrmType.FAIRPLAY in drm_type:
            SubElement(req_drm_system_list, "cpix:DRMSystem", {"kid": key_map[track], "systemId": _fairplay_system_id})
        if DrmType.NCG in drm_type:
            SubElement(req_drm_system_list, "cpix:DRMSystem", {"kid": key_map[track], "systemId": _ncg_system_id})
        if DrmType.HLS_NCG in drm_type:
            SubElement(req_drm_system_list, "cpix:DRMSystem", {"kid": key_map[track], "systemId": _hls_ncg_system_id})

    return tostring(req_root, encoding='utf8', method="xml")


def parse_response(response_data):
    # Parse CPIX Response
    namespaces = {'cpix': 'urn:dashif:org:cpix', 'pskc': 'urn:ietf:params:xml:ns:keyprov:pskc'}
    res_root = fromstring(response_data.text)
    multidrm_infos = []

    for res_content_key_usage_rule in res_root.findall(".//cpix:ContentKeyUsageRule", namespaces):
        track_type = res_content_key_usage_rule.get("intendedTrackType")
        key_id = res_content_key_usage_rule.get("kid")
        multidrm_infos.append(MultiDrmInfo(key_id=key_id, track_type=track_type))

    for res_content_key in res_root.findall(".//cpix:ContentKey", namespaces):
        for multidrm_info in multidrm_infos:
            if res_content_key.get("kid") == multidrm_info.key_id:
                multidrm_info.iv = res_content_key.get("explicitIV")
                multidrm_info.key = res_content_key.find(".//pskc:PlainValue", namespaces).text

    for res_drm_system in res_root.findall(".//cpix:DRMSystem", namespaces):
        for multidrm_info in multidrm_infos:
            if res_drm_system.get("kid") == multidrm_info.key_id:
                system_id = res_drm_system.get("systemId")
                if system_id == _widevine_system_id:
                    multidrm_info.widevine_pssh = res_drm_system.find("cpix:PSSH", namespaces).text
                    multidrm_info.widevine_pssh_payload\
                        = res_drm_system.find("cpix:ContentProtectionData", namespaces).text
                elif system_id == _playready_system_id:
                    multidrm_info.playready_pssh = res_drm_system.find("cpix:PSSH", namespaces).text
                    multidrm_info.playready_pssh_payload\
                        = res_drm_system.find("cpix:ContentProtectionData", namespaces).text
                elif system_id == _fairplay_system_id:
                    multidrm_info.fairplay_hls_key_uri\
                        = base64.b64decode(res_drm_system.find("cpix:URIExtXKey", namespaces).text).decode('utf-8')
                    multidrm_info.fairplay_hls_signaling_data\
                        = base64.b64decode(res_drm_system.find("cpix:HLSSignalingData", namespaces).text).decode('utf-8')
                elif system_id == _ncg_system_id:
                    multidrm_info.ncg_cek = res_drm_system.find("cpix:URIExtXKey", namespaces).text
                elif system_id == _hls_ncg_system_id:
                    multidrm_info.ncg_hls_key_uri\
                        = base64.b64decode(res_drm_system.find("cpix:URIExtXKey", namespaces).text).decode('utf-8')

    pack_info = ContentPackagingInfo(res_root.get("id"))
    pack_info.multidrm_infos = multidrm_infos

    return pack_info


class CpixClient:
    def __init__(self, kms_url, enc_token):
        self._kms_url = kms_url
        self._enc_token = enc_token

    def get_content_key_info_from_pallycon_kms(self, content_id, drm_type
                                               , encryption_scheme=EncryptionScheme.CENC
                                               , track_type=TrackType.ALL_TRACKS):
        # Set PallyCon KMS server URL
        url = f"{self._kms_url}{self._enc_token}"
        # Get request body data
        cpix_request_data = get_request_data(content_id, drm_type, encryption_scheme, track_type)
        # Request and receive response data
        cpix_response_data = requests.post(url, data=cpix_request_data)

        # Check response
        if cpix_response_data.status_code == 200:
            return parse_response(cpix_response_data)
        else:
            print("Error occurred: ", cpix_response_data.text)
            return None
