
#include <string.h>
#include <random>
#include <algorithm>

#include "CpixClient.h"
#include "CpixRequester.h"
#include "helper/xmlParser.h"
#include "helper/base64.h"

namespace pallycon {
#define UUID_SIZE_INCLUDING_NULL_CHAR 37

#define WIDEVINE_SYSMTE_ID	"EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED"
#define PLAYREADY_SYSMTE_ID	"9A04F079-9840-4286-AB92-E65BE0885F95"
#define FAIRPLAY_SYSMTE_ID	"94CE86FB-07FF-4F43-ADB8-93D2FA968CA2"
#define NCG_SYSMTE_ID		"D9E4411A-E886-4909-A380-A77F28D52335"
#define HLS_NCG_SYSMTE_ID	"48582A1D-1FF4-426E-8CD5-06424FCC578C"

	static const char* __encryption_scheme_str[] = { "cenc", "cbc1", "cens", "cbcs" };
	static const TrackType AllTrackTypes[] = { AUDIO, SD, HD, UHD1, UHD2 };

	std::string __Base64Encode(unsigned char* buffer, int length)
	{
		int nOutputLength;
		const char* encodedData = Base64Encode(buffer, length, &nOutputLength);
		std::string resultString;
		resultString.resize(nOutputLength);
		std::copy(&encodedData[0], &encodedData[nOutputLength], resultString.begin());
		delete[] encodedData;

		return resultString;
	}

	std::shared_ptr<BYTE> __Base64Decode(const char* buffer, int* outputLength)
	{
		LPBYTE bytePtr = Base64Decode(buffer, outputLength);
		if (bytePtr == NULL)
		{
			throw CpixClientException("base64Decode() Failed!");
		}
		std::shared_ptr<BYTE> smartPtr(bytePtr, std::default_delete<BYTE[]>());
		return smartPtr;
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

	CpixClient::CpixClient(std::string strKmsURL, std::string strEncToken)
	{
		_kmsUrl = strKmsURL;
		_encToken = strEncToken;
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
		if(periodIndex > 0)
			reqContentKeyPeriodList = reqRoot.addChild("cpix:ContentKeyPeriodList");

		for (auto& map : _keyMap)
		{
			XMLNode reqContentKey = reqContentKeyList.addChild("cpix:ContentKey");
			reqContentKey.addAttribute("kid", map.second.c_str());
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
				reqWidevineNode.addAttribute("systemId", WIDEVINE_SYSMTE_ID);
			}

			if (drmType & PLAYREADY)
			{
				XMLNode reqPlayReadyNode = reqDRMList.addChild("cpix:DRMSystem");

				reqPlayReadyNode.addAttribute("kid", map.second.c_str());
				reqPlayReadyNode.addAttribute("systemId", PLAYREADY_SYSMTE_ID);
			}

			if (drmType & FAIRPLAY)
			{
				XMLNode reqFairPlayNode = reqDRMList.addChild("cpix:DRMSystem");

				reqFairPlayNode.addAttribute("kid", map.second.c_str());
				reqFairPlayNode.addAttribute("systemId", FAIRPLAY_SYSMTE_ID);
			}

			if (drmType & NCG)
			{
				XMLNode reqNcgNode = reqDRMList.addChild("cpix:DRMSystem");

				reqNcgNode.addAttribute("kid", map.second.c_str());
				reqNcgNode.addAttribute("systemId", NCG_SYSMTE_ID);
			}

			if (drmType & HLS_NCG)
			{
				XMLNode reqHlsNcgNode = reqDRMList.addChild("cpix:DRMSystem");

				reqHlsNcgNode.addAttribute("kid", map.second.c_str());
				reqHlsNcgNode.addAttribute("systemId", HLS_NCG_SYSMTE_ID);
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

		std::string systemId_widevine = WIDEVINE_SYSMTE_ID;
		std::string systemId_playready = PLAYREADY_SYSMTE_ID;
		std::string systemId_fairplay = FAIRPLAY_SYSMTE_ID;
		std::string systemId_ncg = NCG_SYSMTE_ID;
		std::string systemId_hlsNcg = HLS_NCG_SYSMTE_ID;

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
				XMLCSTR resKey = resContentKey.getChildNode("cpix:Data").getChildNode("pskc:Secret").getChildNode("pskc:PlainValue").getText();
				drmInfo.key = resKey;
				drmInfo.iv = resIv;
			}

			XMLNode resWidevineNode, resPlayReadyNode, resFairPlayNode, resNcgNode, reqHlsNcgNode;
			for (int i = 0; i < resDRMSystemList.nChildNode(); i++)
			{
				XMLNode resDrmNode = resDRMSystemList.getChildNode("cpix:DRMSystem", i);
				XMLCSTR resSystemId = resDrmNode.getAttribute("systemId");
				if (0 == strcmp(resKeyId, resDrmNode.getAttribute("kid")))
				{
					if (0 == strcmp(resSystemId, systemId_widevine.c_str()))
					{
						resWidevineNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemId_playready.c_str()))
					{
						resPlayReadyNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemId_fairplay.c_str()))
					{
						resFairPlayNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemId_ncg.c_str()))
					{
						resNcgNode = resDrmNode.deepCopy();
					}
					else if (0 == strcmp(resSystemId, systemId_hlsNcg.c_str()))
					{
						reqHlsNcgNode = resDrmNode.deepCopy();
					}
				}
			}

			if (!resWidevineNode.isEmpty())
			{
				drmInfo.widevinePSSHpayload = resWidevineNode.getChildNode("cpix:ContentProtectionData").getText();
				drmInfo.widevinePSSH = resWidevineNode.getChildNode("cpix:PSSH").getText();
			}
			if (!resPlayReadyNode.isEmpty())
			{
				drmInfo.playreadyPSSHpayload = resPlayReadyNode.getChildNode("cpix:ContentProtectionData").getText();
				drmInfo.playreadyPSSH = resPlayReadyNode.getChildNode("cpix:PSSH").getText();
			}

			if (!resFairPlayNode.isEmpty())
			{
				drmInfo.fairplayHlsSignalingData = resFairPlayNode.getChildNode("cpix:HLSSignalingData").getText();
				int outputLength = 0;
				std::shared_ptr<BYTE> keyUri = __Base64Decode(resFairPlayNode.getChildNode("cpix:URIExtXKey").getText(), &outputLength);
				std::string strKeyUri(keyUri.get(), keyUri.get() + outputLength);
				drmInfo.fairplayHlsKeyUri = strKeyUri;
			}

			if (!resNcgNode.isEmpty())
			{
				int outputLength = 0;
				std::shared_ptr<BYTE> ncgCek = __Base64Decode(resNcgNode.getChildNode("cpix:URIExtXKey").getText(), &outputLength);
				std::string strCek(ncgCek.get(), ncgCek.get() + outputLength);
				drmInfo.ncgCek = strCek.c_str();
			}

			if (!reqHlsNcgNode.isEmpty())
			{
				int outputLength = 0;
				std::shared_ptr<BYTE> keyUri = __Base64Decode(reqHlsNcgNode.getChildNode("cpix:URIExtXKey").getText(), &outputLength);
				std::string strKeyUri(keyUri.get(), keyUri.get() + outputLength);
				drmInfo.ncgHlsKeyUri = strKeyUri.c_str();
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
		CpixRequester client(_kmsUrl, _encToken, httpRequester);
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
