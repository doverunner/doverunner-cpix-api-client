
#include <string.h>
#include <random>
#include <algorithm>

#include "CpixClient.h"
#include "CpixRequester.h"
#include "helper/xmlParser.h"
#include "helper/base64.h"

namespace pallycon {
#define UUID_SIZE_INCLUDING_NULL_CHAR 37

#define WIDEVINE_SYSTEM_ID			"EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED"
#define PLAYREADY_SYSTEM_ID			"9A04F079-9840-4286-AB92-E65BE0885F95"
#define FAIRPLAY_SYSTEM_ID			"94CE86FB-07FF-4F43-ADB8-93D2FA968CA2"
#define WISEPLAY_SYSTEM_ID			"3D5E6D35-9B9A-41E8-B843-DD3C6E72C42C"
#define NCG_SYSTEM_ID				"D9E4411A-E886-4909-A380-A77F28D52335"
#define NCGHLS_AES128_SYSTEM_ID		"81376844-F976-481E-A84E-CC25D39B0B33"
#define NCGHLS_SAMPLEAES_SYSTEM_ID	"48582A1D-1FF4-426E-8CD5-06424FCC578C"
#define AES128_SYSTEM_ID			"3EA8778F-7742-4BF9-B18B-E834B2ACBD47"
#define SAMPLEAES_SYSTEM_ID			"BE58615B-19C4-4684-88B3-C8C57E99E957"

	static const char* __encryption_scheme_str[] = { "", "cenc", "cbc1", "cens", "cbcs" };
	static const TrackType AllTrackTypes[] = { AUDIO, SD, HD, UHD1, UHD2 };

	std::shared_ptr<BYTE> __Base64Decode(const char* buffer, int* outputLength)
	{
		LPBYTE bytePtr = Base64Decode(buffer, outputLength);
		if (bytePtr == NULL)
		{
			throw CpixClientException("__Base64Decode : Base64 decoding Failed.");
		}
		std::shared_ptr<BYTE> smartPtr(bytePtr, std::default_delete<BYTE[]>());
		return smartPtr;
	}

	std::string __Base64DecodeToString(const char* buffer)
	{
		int outputLength = 0;
		std::shared_ptr<BYTE> decodedData = __Base64Decode(buffer, &outputLength);
		return std::string(decodedData.get(), decodedData.get() + outputLength);
	}

	void __GenerateUUID(char* pUUID)
	{
		int t = 0;
		char szTemp[] = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
		char szHex[] = "0123456789abcdef-";
		int nLen = strlen(szTemp);

		std::random_device rd;
		// Init randomize engine by using random_device
		std::mt19937 gen(rd());
		// Set randomize 0 ~ 99
		std::uniform_int_distribution<int> dis(0, 99);

		for (t = 0; t < nLen; t++)
		{
			int r = dis(gen) % 16;
			char c = ' ';

			switch (szTemp[t])
			{
			case 'x':
				c = szHex[r];
				break;
			case 'y':
				c = szHex[r & 0x03 | 0x08];
				break;
			case '-':
				c = '-';
				break;
			case '4':
				c = '4';
				break;
			default:
				break;
			}
			pUUID[t] = c;
		}
		pUUID[t] = 0;
	}

	XMLCSTR __GetTextSafely(const XMLNode& node) {
		if (node.isEmpty()) return "";
		return node.getText();
	}

	CpixClient::CpixClient(std::string kmsUrl)
	{
		_kmsUrl = kmsUrl;
		_lastRequestStatus = 0;
	}

	CpixClient::~CpixClient()
	{
	}

	std::string CpixClient::GetRequestData(std::string contentId, DrmType drmType, EncryptionScheme encryptionScheme, TrackType trackType, long periodIndex)
	{
		_keyMap.clear();
		std::string requestData = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

		// Setting the key map
		if (trackType == ALL_TRACKS)
		{
			// Set the track type to "HD" for single-key
			std::map<std::string, std::string> m_single;
			char randomUuid[UUID_SIZE_INCLUDING_NULL_CHAR];
			__GenerateUUID(randomUuid);
			_keyMap.insert(std::pair<std::string, std::string>(TrackTypeToString(HD), randomUuid));
		}
		else
		{
			for (auto it : AllTrackTypes)
			{
				// Generate only maps that correspond to the trackType
				if (it & trackType)
				{
					char randomUuid[UUID_SIZE_INCLUDING_NULL_CHAR];
					__GenerateUUID(randomUuid);
					_keyMap.insert(std::pair<std::string, std::string>(TrackTypeToString(it), randomUuid));
				}
			}
		}

		// Make XML request data
		XMLNode reqRoot = XMLNode::createXMLTopNode("cpix:CPIX");
		reqRoot.addAttribute("id", contentId.c_str());
		reqRoot.addAttribute("xmlns:cpix", "urn:dashif:org:cpix");
		reqRoot.addAttribute("xmlns:pskc", "urn:ietf:params:xml:ns:keyprov:pskc");
		reqRoot.addAttribute("xmlns:speke", "urn:aws:amazon:com:speke");

		XMLNode reqContentKeyList = reqRoot.addChild("cpix:ContentKeyList");
		XMLNode reqDRMList = reqRoot.addChild("cpix:DRMSystemList");
		XMLNode reqContentKeyUsageRuleList = reqRoot.addChild("cpix:ContentKeyUsageRuleList");
		XMLNode reqContentKeyPeriodList;
		if (periodIndex > 0)
			reqContentKeyPeriodList = reqRoot.addChild("cpix:ContentKeyPeriodList");

		for (auto& map : _keyMap)
		{
			XMLNode reqContentKey = reqContentKeyList.addChild("cpix:ContentKey");
			reqContentKey.addAttribute("kid", map.second.c_str());
			if (encryptionScheme != (EncryptionScheme)NONE)
				reqContentKey.addAttribute("commonEncryptionScheme", __encryption_scheme_str[encryptionScheme]);

			XMLNode reqContentKeyUsageRule = reqContentKeyUsageRuleList.addChild("cpix:ContentKeyUsageRule");
			reqContentKeyUsageRule.addAttribute("intendedTrackType", map.first.c_str());
			reqContentKeyUsageRule.addAttribute("kid", map.second.c_str());

			if (periodIndex > 0)
			{
				std::string keyPeriodId = "keyPeriod_";
				char randomUuid[UUID_SIZE_INCLUDING_NULL_CHAR];
				__GenerateUUID(randomUuid);
				keyPeriodId.append(randomUuid);

				XMLNode reqContentKeyPeriod = reqContentKeyPeriodList.addChild("cpix:ContentKeyPeriod");
				reqContentKeyPeriod.addAttribute("id", keyPeriodId.c_str());
				reqContentKeyPeriod.addAttribute("index", std::to_string(periodIndex).c_str());

				XMLNode reqKeyPeriodFilter = reqContentKeyUsageRule.addChild("cpix:KeyPeriodFilter");
				reqKeyPeriodFilter.addAttribute("periodId", keyPeriodId.c_str());
			}

			if (drmType & WIDEVINE)
			{
				XMLNode reqWidevineNode = reqDRMList.addChild("cpix:DRMSystem");

				reqWidevineNode.addAttribute("kid", map.second.c_str());
				reqWidevineNode.addAttribute("systemId", WIDEVINE_SYSTEM_ID);
			}

			if (drmType & PLAYREADY)
			{
				XMLNode reqPlayReadyNode = reqDRMList.addChild("cpix:DRMSystem");

				reqPlayReadyNode.addAttribute("kid", map.second.c_str());
				reqPlayReadyNode.addAttribute("systemId", PLAYREADY_SYSTEM_ID);
			}

			if (drmType & FAIRPLAY)
			{
				XMLNode reqFairPlayNode = reqDRMList.addChild("cpix:DRMSystem");

				reqFairPlayNode.addAttribute("kid", map.second.c_str());
				reqFairPlayNode.addAttribute("systemId", FAIRPLAY_SYSTEM_ID);
			}

			if (drmType & WISEPLAY)
			{
				XMLNode reqWisePlayNode = reqDRMList.addChild("cpix:DRMSystem");

				reqWisePlayNode.addAttribute("kid", map.second.c_str());
				reqWisePlayNode.addAttribute("systemId", WISEPLAY_SYSTEM_ID);
			}

			if (drmType & NCG)
			{
				XMLNode reqNcgNode = reqDRMList.addChild("cpix:DRMSystem");

				reqNcgNode.addAttribute("kid", map.second.c_str());
				reqNcgNode.addAttribute("systemId", NCG_SYSTEM_ID);
			}

			if (drmType & NCGHLS_AES128)
			{
				XMLNode reqNcghlsAes128Node = reqDRMList.addChild("cpix:DRMSystem");

				reqNcghlsAes128Node.addAttribute("kid", map.second.c_str());
				reqNcghlsAes128Node.addAttribute("systemId", NCGHLS_AES128_SYSTEM_ID);
			}

			if (drmType & NCGHLS_SAMPLEAES)
			{
				XMLNode reqNcghlsSampleAesNode = reqDRMList.addChild("cpix:DRMSystem");

				reqNcghlsSampleAesNode.addAttribute("kid", map.second.c_str());
				reqNcghlsSampleAesNode.addAttribute("systemId", NCGHLS_SAMPLEAES_SYSTEM_ID);
			}

			if (drmType & AES128)
			{
				XMLNode reqAes128Node = reqDRMList.addChild("cpix:DRMSystem");

				reqAes128Node.addAttribute("kid", map.second.c_str());
				reqAes128Node.addAttribute("systemId", AES128_SYSTEM_ID);
			}

			if (drmType & SAMPLEAES)
			{
				XMLNode reqSampleAesNode = reqDRMList.addChild("cpix:DRMSystem");

				reqSampleAesNode.addAttribute("kid", map.second.c_str());
				reqSampleAesNode.addAttribute("systemId", SAMPLEAES_SYSTEM_ID);
			}
		}

		requestData.append(reqRoot.createXMLString());

		return requestData;
	}

	ContentPackagingInfo CpixClient::ParseResponse(const std::string& responseBody)
	{
		ContentPackagingInfo packInfo;
		XMLNode responseRoot = XMLNode::parseString(responseBody.c_str(), "cpix:CPIX");

		if (responseRoot.isEmpty())
		{
			std::string errMsg;
			errMsg = "CpixClient::parseResponse() : Response parsing failed. Response body: \n" + responseBody;
			throw CpixClientException(errMsg);
		}

		if (responseRoot.getAttribute("id") == NULL)
			throw CpixClientException("CpixClient::parseResponse() : No CID in response.");

		packInfo.contentId = responseRoot.getAttribute("id");

		std::string systemIdWidevine = WIDEVINE_SYSTEM_ID;
		std::string systemIdPlayready = PLAYREADY_SYSTEM_ID;
		std::string systemIdFairplay = FAIRPLAY_SYSTEM_ID;
		std::string systemIdWiseplay = WISEPLAY_SYSTEM_ID;
		std::string systemIdNcg = NCG_SYSTEM_ID;
		std::string systemIdNcghlsAes128 = NCGHLS_AES128_SYSTEM_ID;
		std::string systemIdNcghlsSampleAes = NCGHLS_SAMPLEAES_SYSTEM_ID;
		std::string systemIdAes128 = AES128_SYSTEM_ID;
		std::string systemIdSampleAes = SAMPLEAES_SYSTEM_ID;

		XMLNode resContentKeyList = responseRoot.getChildNode("cpix:ContentKeyList");
		XMLNode resContentKeyUsageRuleList = responseRoot.getChildNode("cpix:ContentKeyUsageRuleList");
		XMLNode resDRMSystemList = responseRoot.getChildNode("cpix:DRMSystemList");

		int idx = 0;
		for (auto& map : _keyMap) // Response key count is same with request key count
		{
			MultiDrmInfo drmInfo;
			XMLNode resContentKeyUsageRule = resContentKeyUsageRuleList.getChildNode("cpix:ContentKeyUsageRule", idx++);
			XMLCSTR resTrackType = resContentKeyUsageRule.getAttribute("intendedTrackType");
			XMLCSTR resKeyId = resContentKeyUsageRule.getAttribute("kid");
			std::string keyId = resKeyId;
			keyId.erase(std::remove(keyId.begin(), keyId.end(), '-'), keyId.end()); // remove hyphen('-')
			transform(keyId.begin(), keyId.end(), keyId.begin(), ::toupper);

			drmInfo.trackType = resTrackType;
			drmInfo.keyId = keyId;

			XMLNode resKeyPeriodFilter = resContentKeyUsageRule.getChildNode("cpix:KeyPeriodFilter", 0);
			if (!resKeyPeriodFilter.isEmpty())
			{
				XMLCSTR resPeriodId = resKeyPeriodFilter.getAttribute("periodId");
				XMLNode resContentKeyPeriodList = responseRoot.getChildNode("cpix:ContentKeyPeriodList");
				for (int i = 0; i < resContentKeyPeriodList.nChildNode(); i++)
				{
					XMLNode resContentKeyPeriod = resContentKeyPeriodList.getChildNode("cpix:ContentKeyPeriod", i);
					if (0 == strcmp(resPeriodId, resContentKeyPeriod.getAttribute("id")))
					{
						drmInfo.periodIndex = resContentKeyPeriod.getAttribute("index");
					}
				}
			}

			XMLNode resContentKey;
			for (int i = 0; i < resContentKeyList.nChildNode(); i++)
			{
				XMLNode resContentKeyIter = resContentKeyList.getChildNode("cpix:ContentKey", i);
				if (!resContentKeyIter.isEmpty())
				{
					if (strcmp(resKeyId, resContentKeyIter.getAttribute("kid")) == 0)
					{
						resContentKey = resContentKeyIter.deepCopy();
						break;
					}
				}
			}

			if (!resContentKey.isEmpty())
			{
				XMLCSTR resIv = resContentKey.getAttribute("explicitIV");
				XMLCSTR resKey = __GetTextSafely(resContentKey.getChildNode("cpix:Data").getChildNode("pskc:Secret").getChildNode("pskc:PlainValue"));
				drmInfo.key = resKey;
				drmInfo.iv = resIv;
			}

			XMLNode resWidevineNode, resPlayReadyNode, resFairPlayNode, resWisePlayNode, resNcgNode, resNcghlsAes128Node, resNcghlsSampleAesNode, resAes128Node, resSampleAesNode;
			for (int i = 0; i < resDRMSystemList.nChildNode(); i++)
			{
				XMLNode resDrmNode = resDRMSystemList.getChildNode("cpix:DRMSystem", i);
				XMLCSTR resSystemId = resDrmNode.getAttribute("systemId");
				if (0 == strcmp(resKeyId, resDrmNode.getAttribute("kid")))
				{
					if (0 == strcmp(resSystemId, systemIdWidevine.c_str()))
					{
						resWidevineNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdPlayready.c_str()))
					{
						resPlayReadyNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdFairplay.c_str()))
					{
						resFairPlayNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdWiseplay.c_str()))
					{
						resWisePlayNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdNcg.c_str()))
					{
						resNcgNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdNcghlsAes128.c_str()))
					{
						resNcghlsAes128Node = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdNcghlsSampleAes.c_str()))
					{
						resNcghlsSampleAesNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdAes128.c_str()))
					{
						resAes128Node = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemIdSampleAes.c_str()))
					{
						resSampleAesNode = resDrmNode.deepCopy();
					}
				}
			}

			if (!resWidevineNode.isEmpty())
			{
				drmInfo.widevinePSSHpayload = __GetTextSafely(resWidevineNode.getChildNode("cpix:ContentProtectionData"));
				drmInfo.widevinePSSH = __GetTextSafely(resWidevineNode.getChildNode("cpix:PSSH"));
				drmInfo.widevineHlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resWidevineNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.widevineHlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resWidevineNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}
			if (!resPlayReadyNode.isEmpty())
			{
				drmInfo.playreadyPSSHpayload = __GetTextSafely(resPlayReadyNode.getChildNode("cpix:ContentProtectionData"));
				drmInfo.playreadyPSSH = __GetTextSafely(resPlayReadyNode.getChildNode("cpix:PSSH"));
				drmInfo.playreadySmoothStreamingData = __GetTextSafely(resPlayReadyNode.getChildNode("cpix:SmoothStreamingProtectionHeaderData"));
				drmInfo.playreadyHlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resPlayReadyNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.playreadyHlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resPlayReadyNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			if (!resFairPlayNode.isEmpty())
			{
				drmInfo.fairplayHlsKeyUri = __Base64DecodeToString(__GetTextSafely(resFairPlayNode.getChildNode("cpix:URIExtXKey")));
				drmInfo.fairplayHlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resFairPlayNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.fairplayHlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resFairPlayNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			if (!resWisePlayNode.isEmpty())
			{
				drmInfo.wiseplayPSSHpayload = __GetTextSafely(resWisePlayNode.getChildNode("cpix:ContentProtectionData"));
				drmInfo.wiseplayPSSH = __GetTextSafely(resWisePlayNode.getChildNode("cpix:PSSH"));
				drmInfo.wiseplayHlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resWisePlayNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.wiseplayHlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resWisePlayNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			if (!resNcgNode.isEmpty())
			{
				int outputLength = 0;
				std::shared_ptr<BYTE> ncgCek = __Base64Decode(__GetTextSafely(resNcgNode.getChildNode("cpix:URIExtXKey")), &outputLength);
				std::string strCek(ncgCek.get(), ncgCek.get() + outputLength);
				drmInfo.ncgCek = strCek.c_str();
			}

			if (!resNcghlsAes128Node.isEmpty())
			{
				drmInfo.ncghlsAes128KeyUri = __Base64DecodeToString(__GetTextSafely(resNcghlsAes128Node.getChildNode("cpix:URIExtXKey")));
				drmInfo.ncghlsAes128HlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resNcghlsAes128Node.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.ncghlsAes128HlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resNcghlsAes128Node.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			if (!resNcghlsSampleAesNode.isEmpty())
			{
				drmInfo.ncghlsSampleAesKeyUri = __Base64DecodeToString(__GetTextSafely(resNcghlsSampleAesNode.getChildNode("cpix:URIExtXKey")));
				drmInfo.ncghlsSampleAesHlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resNcghlsSampleAesNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.ncghlsSampleAesHlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resNcghlsSampleAesNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			if (!resAes128Node.isEmpty())
			{
				drmInfo.aes128KeyUri = __Base64DecodeToString(__GetTextSafely(resAes128Node.getChildNode("cpix:URIExtXKey")));
				drmInfo.aes128HlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resAes128Node.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.aes128HlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resAes128Node.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			if (!resSampleAesNode.isEmpty())
			{
				drmInfo.sampleAesKeyUri = __Base64DecodeToString(__GetTextSafely(resSampleAesNode.getChildNode("cpix:URIExtXKey")));
				drmInfo.sampleAesHlsSignalingDataMaster = __Base64DecodeToString(__GetTextSafely(resSampleAesNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "master")));
				drmInfo.sampleAesHlsSignalingDataMedia = __Base64DecodeToString(__GetTextSafely(resSampleAesNode.getChildNodeWithAttribute("cpix:HLSSignalingData", "playlist", "media")));
			}

			packInfo.multiDrmInfos.push_back(drmInfo);
		}

		return packInfo;
	}

	ContentPackagingInfo CpixClient::GetContentKeyInfoFromPallyConKMS(const std::string contentId
		, DrmType drmType, EncryptionScheme encryptionScheme, TrackType trackType, long periodIndex)
	{
		if (contentId.empty())
		{
			throw CpixClientException("CpixClient::getContentPackagingInfoFromKmsServer() : invalid parameter.");
		}

		std::shared_ptr<CurlHttpRequester> httpRequester(new CurlHttpRequester());
		CpixRequester client(_kmsUrl, httpRequester);
		_lastRequestRowData = GetRequestData(contentId, drmType, encryptionScheme, trackType, periodIndex);
		client.SetRequestData(_lastRequestRowData);
		std::string responseRowData = client.Request();
		if (responseRowData == "")
		{
			_lastRequestStatus = httpRequester->getRequestStatus();
			std::string errorMessage = "CpixRequester:request() : " + httpRequester->getReason();
			throw CpixClientException(errorMessage.c_str());
		}
		_lastResponseRowData = responseRowData;

		ContentPackagingInfo packInfo = ParseResponse(responseRowData);

		if (trackType == ALL_TRACKS)
		{
			// Single key
			packInfo.multiDrmInfos[0].trackType = "ALL_TRACKS";
		}

		return packInfo;
	}
}
