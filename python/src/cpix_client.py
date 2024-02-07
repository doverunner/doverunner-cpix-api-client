import requests
import uuid
import base64
import json
from xml.etree.ElementTree import Element, SubElement, tostring, fromstring
from drm_type import DrmType
from encryption_scheme import EncryptionScheme
from track_type import TrackType
from content_packaging_data import ContentPackagingInfo, MultiDrmInfo
from exceptions import CpixClientError

_widevine_system_id = "EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED"
_playready_system_id = "9A04F079-9840-4286-AB92-E65BE0885F95"
_fairplay_system_id = "94CE86FB-07FF-4F43-ADB8-93D2FA968CA2"
_ncg_system_id = "D9E4411A-E886-4909-A380-A77F28D52335"
_hls_ncg_system_id = "48582A1D-1FF4-426E-8CD5-06424FCC578C"


def get_request_data(content_id, drm_type, encryption_scheme, track_type, period_index):
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
    req_drm_system_list = SubElement(req_root, "cpix:DRMSystemList")
    req_content_key_usage_rule_list = SubElement(req_root, "cpix:ContentKeyUsageRuleList")
    req_content_key_period_list = None
    if period_index > 0:
        req_content_key_period_list = SubElement(req_root, "cpix:ContentKeyPeriodList")

    for track in key_map:
        SubElement(req_content_key_list, "cpix:ContentKey",
                   {"kid": key_map[track], "commonEncryptionScheme": encryption_scheme.name.lower()})
        req_content_key_usage_rule = SubElement(req_content_key_usage_rule_list, "cpix:ContentKeyUsageRule",
                                                {"intendedTrackType": track, "kid": key_map[track]})

        if period_index > 0:
            key_period_id = "keyPeriod_" + str(uuid.uuid4())
            SubElement(req_content_key_period_list, "cpix:ContentKeyPeriod",
                       {"id": key_period_id, "index": str(period_index)})
            SubElement(req_content_key_usage_rule, "cpix:KeyPeriodFilter", {"periodId": key_period_id})

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


def is_valid_response(response_data):
    # This is because if the KMS server returns a custom error code, it will respond in JSON format.
    result = False
    try:
        json.loads(response_data.text)
    except json.JSONDecodeError:
        result = True

    return result


def parse_response(response_data):
    # Parse CPIX Response
    namespaces = {'cpix': 'urn:dashif:org:cpix', 'pskc': 'urn:ietf:params:xml:ns:keyprov:pskc'}
    res_root = fromstring(response_data.text)
    multidrm_infos = []

    for res_content_key_usage_rule in res_root.findall(".//cpix:ContentKeyUsageRule", namespaces):
        multidrm_info = MultiDrmInfo()

        track_type = res_content_key_usage_rule.get("intendedTrackType")
        key_id = res_content_key_usage_rule.get("kid")
        multidrm_info.key_id = key_id
        multidrm_info.track_type = track_type

        res_key_period_filter = res_content_key_usage_rule.find(".//cpix:KeyPeriodFilter", namespaces)
        if res_key_period_filter is not None:
            period_id = res_key_period_filter.get("periodId")
            for res_content_key_period in res_root.findall(".//cpix:ContentKeyPeriod", namespaces):
                if period_id == res_content_key_period.get("id"):
                    multidrm_info.period_index = res_content_key_period.get("index")
                    break
        multidrm_infos.append(multidrm_info)

    for res_content_key in res_root.findall(".//cpix:ContentKey", namespaces):
        for multidrm_info in multidrm_infos:
            if multidrm_info.key_id == res_content_key.get("kid"):
                multidrm_info.iv = res_content_key.get("explicitIV")
                multidrm_info.key = res_content_key.find(".//pskc:PlainValue", namespaces).text

    for res_drm_system in res_root.findall(".//cpix:DRMSystem", namespaces):
        for multidrm_info in multidrm_infos:
            if multidrm_info.key_id == res_drm_system.get("kid"):
                system_id = res_drm_system.get("systemId")
                if system_id == _widevine_system_id:
                    multidrm_info.widevine_pssh = res_drm_system.find("cpix:PSSH", namespaces).text
                    multidrm_info.widevine_pssh_payload \
                        = res_drm_system.find("cpix:ContentProtectionData", namespaces).text
                elif system_id == _playready_system_id:
                    multidrm_info.playready_pssh = res_drm_system.find("cpix:PSSH", namespaces).text
                    multidrm_info.playready_pssh_payload \
                        = res_drm_system.find("cpix:ContentProtectionData", namespaces).text
                elif system_id == _fairplay_system_id:
                    multidrm_info.fairplay_hls_key_uri \
                        = base64.b64decode(res_drm_system.find("cpix:URIExtXKey", namespaces).text).decode('utf-8')
                    multidrm_info.fairplay_hls_signaling_data \
                        = base64.b64decode(res_drm_system.find("cpix:HLSSignalingData", namespaces).text).decode(
                        'utf-8')
                elif system_id == _ncg_system_id:
                    multidrm_info.ncg_cek = res_drm_system.find("cpix:URIExtXKey", namespaces).text
                elif system_id == _hls_ncg_system_id:
                    multidrm_info.ncg_hls_key_uri \
                        = base64.b64decode(res_drm_system.find("cpix:URIExtXKey", namespaces).text).decode('utf-8')

    pack_info = ContentPackagingInfo(res_root.get("id"))
    pack_info.multidrm_infos = multidrm_infos

    return pack_info


class CpixClient:
    def __init__(self, kms_url):
        self._kms_url = kms_url

    def get_content_key_info_from_pallycon_kms(self, content_id, drm_type
                                               , encryption_scheme=EncryptionScheme.CENC
                                               , track_type=TrackType.ALL_TRACKS
                                               , period_index=0):
        # Set PallyCon KMS server URL
        url = f"{self._kms_url}"
        # Get request body data
        cpix_request_data = get_request_data(content_id, drm_type, encryption_scheme, track_type, period_index)

        try:
            # Request and receive response data
            cpix_response_data = requests.post(url, data=cpix_request_data)
        except requests.exceptions.HTTPError as err_http:
            raise CpixClientError("An Http Error occurred: " + err_http.response.text)
        except requests.exceptions.ConnectionError as err_conn:
            raise CpixClientError("An Error Connecting to the API occurred: " + err_conn.response.text)
        except requests.exceptions.Timeout as err_timeout:
            raise CpixClientError("A Timeout Error occurred: " + err_timeout.response.text)

        # Check response
        if cpix_response_data.status_code == 200:
            if is_valid_response(cpix_response_data):
                return parse_response(cpix_response_data)
            else:
                raise CpixClientError(cpix_response_data.text)
        else:
            raise CpixClientError("Error occurred: ", cpix_response_data.text)
