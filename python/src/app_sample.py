import json
import base64
from uuid import UUID
from cpix_client import CpixClient, DrmType, EncryptionScheme, TrackType
from exceptions import CpixClientError


def make_json_string_from_data(pack_info):
    str_json = {"content_id": pack_info.content_id, "content_key_list": []}
    for multidrm_info in pack_info.multidrm_infos:
        str_multidrm_info = {}

        if len(pack_info.multidrm_infos) > 1:
            str_multidrm_info["track_type"] = multidrm_info.track_type

        str_multidrm_info.update({
            "key_id_hex": UUID(multidrm_info.key_id).hex,
            "key_id_b64": base64.b64encode(bytes.fromhex(UUID(multidrm_info.key_id).hex)).decode(),
            "key_hex": base64.b64decode(multidrm_info.key.encode()).hex(),
            "key_b64": multidrm_info.key,
            "iv_hex": base64.b64decode(multidrm_info.iv.encode()).hex(),
            "iv_b64": multidrm_info.iv,
            "widevine": {
                "pssh": multidrm_info.widevine_pssh,
                "pssh_payload_only": multidrm_info.widevine_pssh_payload
            },
            "playready": {
                "pssh": multidrm_info.playready_pssh,
                "pssh_payload_only": multidrm_info.playready_pssh_payload
            },
            "fairplay": {
                "key_uri": multidrm_info.fairplay_hls_key_uri
            }
        })

        str_json["content_key_list"].append(str_multidrm_info)

    return str_json


def main():
    kms_url = "https://kms.pallycon.com/v2/cpix/pallycon/getKey/{enc-token}" # Put your KMS enc-token
    content_id = ""  # Put your content id

    try:
        # Get the packaging information from PallyCon KMS Server
        cpix_client = CpixClient(kms_url)
        pack_info = cpix_client.get_content_key_info_from_pallycon_kms(
            content_id, DrmType.WIDEVINE | DrmType.PLAYREADY | DrmType.FAIRPLAY)

        # Convert data to JSON
        json_pack_info = make_json_string_from_data(pack_info)
        print(json.dumps(json_pack_info, indent=4))

        # Create to file
        output_file = f'./{content_id}.json'
        with open(output_file, 'w', encoding='utf-8') as file:
            json.dump(json_pack_info, file, indent=4)
    except CpixClientError as err_cpix:
        print("CPIX Client error occurred: ", err_cpix)
    except Exception as e:
        print(e)


if __name__ == "__main__":
    main()
