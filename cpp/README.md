---------------------------------------
# PallyCon CPIX Client for C++
 This library shows the client code for CPIX communication with PallyCon KMS server in C++ language.



---------------------------------------
## Requirements

- Visual Studio 2022 (Windows 10/11)

- KMS token

  - This is an API authentication token that is generated when you sign up PallyCon service.

  
---------------------------------------
## How to build and test
1. Clone or download this sample repository.
2. Open the solution file(*pallycon-cpix-client-cpp.sln*) through Visual Studio 2022.
3. Enter the `encToken` and `contentId`, build the project, and run the Sample.
4. If the project runs successfully, it will show the response organized in JSON format on the sample console screen that was run, and also save this JSON data to a file.

   - Originally, CPIX Responses are received in XML format, but <u>for readability, the sample creates them in json format</u>.

   
---------------------------------------
## Function parameters

 You can set the parameters of the function based on the packaging scenario you require. 

```c++
/**
* Receive packaging information from PallyCon KMS server.
*
* @param cid					Content id
* @param drmType				DRM type. (e.g. WIDEVINE|PLAYREADY|FAIRPLAY)
* @param encryptionScheme		Encryption scheme. (e.g. CENC, CBCS, etc)
* @param trackType				Track type for multi-key packaging. (e.g. SD|HD|AUDIO)
*								For single-key packaging, it should be ALL_TRACKS.
*/
ContentPackagingInfo GetContentKeyInfosFromPallyConKMS(const std::string cid, DrmType drmType, EncryptionScheme encryptionScheme = CENC, TrackType trackType = ALL_TRACKS, long periodIndex = 0);
```

***EncryptionScheme*** and ***TrackType*** are optional parameters that default to `CENC` and `ALL_TRACKS`(single-key) when nothing is entered.

If you want to enable key rotation, you can put a value greater than 0 in `periodIndex` which defaults to 0.



```c++
enum EncryptionScheme {
		CENC,
		CBC1,
		CENS,
		CBCS
	};
	
enum DrmType {
	WIDEVINE = (1 << 0),	// 0000 0001 // 0x01
	PLAYREADY = (1 << 1),	// 0000 0010 // 0x02
	FAIRPLAY = (1 << 2),	// 0000 0100 // 0x04
	NCG = (1 << 3),			// 0000 1000 // 0x08
	HLS_NCG = (1 << 4),		// 0001 0000 // 0x10
};

enum TrackType {
	ALL_TRACKS,
	AUDIO = (1 << 0),
	SD = (1 << 1),
	HD = (1 << 2),
	UHD1 = (1 << 3),
	UHD2 = (1 << 4)
};
```

 ***DrmType*** and ***TrackType*** are bit flags with bitwise operations, so you can set multiple values using the OR(`|`) operator, such as `WIDEVINE|PLAYREADY` or `SD|HD|AUDIO`.



> NOTE : NCG and HLS_NCG types are only support single-key.



---------------------------------------

## Output json data format

 The items of the output data includes all data such as hex encoded, base64 encoded, payload_only, etc. so that you can easily copy and paste it into each packaging service.

- **Single-key** sample

```json
{
  "content_id": "demotest",
  "content_key_list": [
    {
      "key_id_hex": "CBBC38B6B50DAB53F8263E06B7382AA3",
      "key_id_b64": "y7w4trUNq1P4Jj4Gtzgqow==",
      "key_hex": "32333232314537424546393634414439",
      "key_b64": "MjMyMjFFN0JFRjk2NEFEOQ==",
      "iv_hex": "30313233343536373839616263646566",
      "iv_b64": "MDEyMzQ1Njc4OWFiY2RlZg==",
      "widevine": {
        "pssh": "AAAATHBzc2gAAAAA7e+LqXnWSs6jyCfc1R0h7QAAACwIARIQy7w4trUNq1P4Jj4GtzgqoxoMaW5rYWVudHdvcmtzIghkZW1vdGVzdA==",
        "pssh_payload_only": "CAESEMu8OLa1DatT+CY+Brc4KqMaDGlua2FlbnR3b3JrcyIIZGVtb3Rlc3Q="
      },
      "playready": {
        "pssh": "AAACknBzc2gAAAAAmgTweZhAQoarkuZb4IhflQAAAnJyAgAAAQABAGgCPABXAFIATQBIAEUAQQBEAEUAUgAgAHgAbQBsAG4AcwA9ACIAaAB0AHQAcAA6AC8ALwBzAGMAaABlAG0AYQBzAC4AbQBpAGMAcgBvAHMAbwBmAHQALgBjAG8AbQAvAEQAUgBNAC8AMgAwADAANwAvADAAMwAvAFAAbABhAHkAUgBlAGEAZAB5AEgAZQBhAGQAZQByACIAIAB2AGUAcgBzAGkAbwBuAD0AIgA0AC4AMwAuADAALgAwACIAPgA8AEQAQQBUAEEAPgA8AFAAUgBPAFQARQBDAFQASQBOAEYATwA+ADwASwBJAEQAUwA+ADwASwBJAEQAIABBAEwARwBJAEQAPQAiAEEARQBTAEMAVABSACIAIABDAEgARQBDAEsAUwBVAE0APQAiAEgARQBzAHYAbABYAFUAeABuAFcAdwA9ACIAIABWAEEATABVAEUAPQAiAHQAagBpADgAeQB3ADIAMQBVADYAdgA0AEoAagA0AEcAdAB6AGcAcQBvAHcAPQA9ACIAPgA8AC8ASwBJAEQAPgA8AC8ASwBJAEQAUwA+ADwALwBQAFIATwBUAEUAQwBUAEkATgBGAE8APgA8AEwAQQBfAFUAUgBMAD4AaAB0AHQAcABzADoALwAvAGwAaQBjAGUAbgBzAGUALgBwAGEAbABsAHkAYwBvAG4ALgBjAG8AbQAvAHIAaQAvAGwAaQBjAGUAbgBzAGUATQBhAG4AYQBnAGUAcgAuAGQAbwA8AC8ATABBAF8AVQBSAEwAPgA8AC8ARABBAFQAQQA+ADwALwBXAFIATQBIAEUAQQBEAEUAUgA+AA==",
        "pssh_payload_only": "cgIAAAEAAQBoAjwAVwBSAE0ASABFAEEARABFAFIAIAB4AG0AbABuAHMAPQAiAGgAdAB0AHAAOgAvAC8AcwBjAGgAZQBtAGEAcwAuAG0AaQBjAHIAbwBzAG8AZgB0AC4AYwBvAG0ALwBEAFIATQAvADIAMAAwADcALwAwADMALwBQAGwAYQB5AFIAZQBhAGQAeQBIAGUAYQBkAGUAcgAiACAAdgBlAHIAcwBpAG8AbgA9ACIANAAuADMALgAwAC4AMAAiAD4APABEAEEAVABBAD4APABQAFIATwBUAEUAQwBUAEkATgBGAE8APgA8AEsASQBEAFMAPgA8AEsASQBEACAAQQBMAEcASQBEAD0AIgBBAEUAUwBDAFQAUgAiACAAQwBIAEUAQwBLAFMAVQBNAD0AIgBIAEUAcwB2AGwAWABVAHgAbgBXAHcAPQAiACAAVgBBAEwAVQBFAD0AIgB0AGoAaQA4AHkAdwAyADEAVQA2AHYANABKAGoANABHAHQAegBnAHEAbwB3AD0APQAiAD4APAAvAEsASQBEAD4APAAvAEsASQBEAFMAPgA8AC8AUABSAE8AVABFAEMAVABJAE4ARgBPAD4APABMAEEAXwBVAFIATAA+AGgAdAB0AHAAcwA6AC8ALwBsAGkAYwBlAG4AcwBlAC4AcABhAGwAbAB5AGMAbwBuAC4AYwBvAG0ALwByAGkALwBsAGkAYwBlAG4AcwBlAE0AYQBuAGEAZwBlAHIALgBkAG8APAAvAEwAQQBfAFUAUgBMAD4APAAvAEQAQQBUAEEAPgA8AC8AVwBSAE0ASABFAEEARABFAFIAPgA="
      },
      "fairplay": {
        "key_uri": "skd://y7w4trUNq1P4Jj4Gtzgqow=="
      }
    }
  ]
}
```



#### The XML value in the CPIX Response that matches the key value in the json data

| Json key                                 | CPIX Response                                         |
| ---------------------------------------- | ----------------------------------------------------- |
| content_id                               | `id` attribute in **`<cpix:CPIX>`** tag               |
| key_id                                   | `kid` attribute in **`<cpix:ContentKey>`** tag        |
| key                                      | Text value of **`<pskc:PlainValue>`** tag             |
| iv                                       | `explicitIV` attribute in **`<cpix:ContentKey>`** tag |
| pssh (Widevine & PlayReady)              | Text value of **`<cpix:PSSH>`** tag                   |
| pssh_payload_only (Widevine & PlayReady) | Text value of **`<cpix:ContentProtectionData>`** tag  |
| key_uri (FairPlay)                       | Text value of **`<cpix:URIExtXKey>`** tag             |

For CPIX Response, please refer to [PallyCon CPIX API Guide](https://pallycon.com/docs/ko/multidrm/packaging/cpix-api/).



---------------------------------------

## References
- https://dashif-documents.azurewebsites.net/Cpix/master/Cpix.html
- https://pallycon.com/docs/en/multidrm/packaging/cpix-api/

---------------------------------------
