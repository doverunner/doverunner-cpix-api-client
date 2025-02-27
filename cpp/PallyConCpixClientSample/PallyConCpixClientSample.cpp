#pragma warning(disable:4996)

#include <iostream>
#include <fstream>  

#include "util.h"
#include "helper/base64.h"
#include "helper/json/json.h"
#include "CpixClient.h"
#include "CpixClientException.h"

#if defined(_WIN32)
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE* __cdecl __iob_func(void)
{
	return _iob;
}
#endif

using namespace pallycon;

std::string FromHexToBase64(std::string strHex)
{
	std::string strBase64;

	unsigned char* bytes = NULL;
	int bytesLength;
	hexToBytes(strHex, bytes, bytesLength);

	int base64Length;
	const char* encodedData = Base64Encode(bytes, bytesLength, &base64Length);
	strBase64.resize(base64Length);
	std::copy(&encodedData[0], &encodedData[base64Length], strBase64.begin());

	if (encodedData != NULL)
		delete[] encodedData;

	if (bytes != NULL)
		delete[] bytes;

	return strBase64;
}

std::string FromBase64ToHex(std::string strBase64)
{
	std::string strHex;

	int bytesLength;
	unsigned char* b64DecodedBytes = Base64Decode(strBase64.c_str(), &bytesLength);
	strHex = bytesToHex(b64DecodedBytes, bytesLength);

	if (b64DecodedBytes != NULL)
		delete[] b64DecodedBytes;

	return strHex;
}

std::string sortJsonStr(Json::Value& root, std::vector<std::string> sortKeys)
{
	// Since the JSON utility used in the sample sorts the key values by CZString comparison, reorder it for readability.
	Json::Value sortedJson;
	sortedJson["content_id"] = root["content_id"];

	int idx = 0;
	for (auto contentKey : root["content_key_list"])
	{
		Json::Value sortedContentKey;
		char newKey[60];
		int i = 0;
		for (auto& key : sortKeys) {
			sprintf(newKey, "SORTEDKEY:%03d-%s", i++, key.c_str());
			sortedContentKey[newKey] = contentKey[key];
		}
		sortedJson["content_key_list"][idx++] = (sortedContentKey);
	}

	std::string result = sortedJson.toStyledString();

	// Remove prefix string
	std::size_t pos = 0;
	while ((pos = result.find("SORTEDKEY:", pos)) != std::string::npos) {
		result.erase(pos, 14);
	}

	// Remove track_type for single-key
	if (idx <= 1) {
		if ((pos = result.find("track_type", 0)) != std::string::npos)
			result.erase(pos - 1, 24);
	}
	return result;
}

std::string MakeJsonStringFromData(ContentPackagingInfo data)
{
	Json::Value root;
	root["content_id"] = data.contentId;
	int idx = 0;
	for (auto drmInfo : data.multiDrmInfos)
	{
		Json::Value contentKeyInfo;

		if (data.multiDrmInfos.size() > 1)
			contentKeyInfo["track_type"] = drmInfo.trackType;

		contentKeyInfo["key_id_hex"] = drmInfo.keyId;
		contentKeyInfo["key_id_b64"] = FromHexToBase64(drmInfo.keyId);
		contentKeyInfo["key_hex"] = FromBase64ToHex(drmInfo.key);
		contentKeyInfo["key_b64"] = drmInfo.key;
		contentKeyInfo["iv_hex"] = FromBase64ToHex(drmInfo.iv);
		contentKeyInfo["iv_b64"] = drmInfo.iv;
		Json::Value widevine;
		widevine["pssh"] = drmInfo.widevinePSSH;
		widevine["pssh_payload_only"] = drmInfo.widevinePSSHpayload;
		contentKeyInfo["widevine"] = (widevine);
		Json::Value playready;
		playready["pssh"] = drmInfo.playreadyPSSH;
		playready["pssh_payload_only"] = drmInfo.playreadyPSSHpayload;
		contentKeyInfo["playready"] = (playready);
		Json::Value fairplay;
		fairplay["key_uri"] = drmInfo.fairplayHlsKeyUri;
		contentKeyInfo["fairplay"] = (fairplay);

		root["content_key_list"][idx++] = (contentKeyInfo);
	}

	// Sort for readability
	std::string sortedJsonString = sortJsonStr(root,
		{ "track_type", "key_id_hex", "key_id_b64", "key_hex", "key_b64", "iv_hex", "iv_b64", "widevine", "playready", "fairplay" });

	return sortedJsonString;
}

int main()
{
	std::string kmsUrl = "https://kms.pallycon.com/v2/cpix/pallycon/getKey/{enc-token}"; // Put your KMS enc-token
	std::string contentId = ""; // Put your content id

	try
	{
		// Get packaging information from PallyCon the KMS Server
		std::shared_ptr<CpixClient> pCpixClient(new CpixClient(kmsUrl));
		ContentPackagingInfo packInfos = pCpixClient->GetContentKeyInfoFromPallyConKMS(contentId, WIDEVINE | PLAYREADY | FAIRPLAY);

		// Convert data to JSON
		std::string strJson = MakeJsonStringFromData(packInfos);
		std::cout << strJson << std::endl;

		// Create to file
		std::string fileName = contentId + ".json";
		std::ofstream outFile(fileName.c_str());
		outFile << strJson << std::endl;
		outFile.close();
	}
	catch (pallycon::CpixClientException& e)
	{
		std::cout << e.what() << std::endl;
	}
	catch (std::exception& e)
	{
		std::cout << "Unknown exception : " << e.what() << std::endl;
	}
}