#pragma once

#include <string>
#include <map>
#include <vector>
#include "CpixClientException.h"

namespace pallycon
{
	enum EncryptionScheme {
		CENC,
		CBC1,
		CENS,
		CBCS
	};

	enum DrmType {
		NONE,
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

	inline const char* TrackTypeToString(TrackType t) {
		switch (t) 
		{
			case AUDIO: return "AUDIO";
			case SD: return "SD";
			case HD: return "HD";
			case UHD1: return "UHD1";
			case UHD2: return "UHD2";
			default: return "Unknown";
		}
	}

	inline DrmType operator&(DrmType type_a, DrmType type_b) {
		return static_cast<DrmType>(static_cast<int>(type_a) & static_cast<int>(type_b));
	}

	inline DrmType operator|(DrmType type_a, DrmType type_b) {
		return static_cast<DrmType>(static_cast<int>(type_a) | static_cast<int>(type_b));
	}

	inline DrmType operator^(DrmType type_a, DrmType type_b) {
		return static_cast<DrmType>(static_cast<int>(type_a) ^ static_cast<int>(type_b));
	}

	inline DrmType& operator&=(DrmType& type_a, DrmType type_b) {
		return (DrmType&)((int&)(type_a) &= (int)(type_b));
	}

	inline DrmType& operator|=(DrmType& type_a, DrmType type_b) {
		return (DrmType&)((int&)(type_a) |= (int)(type_b));
	}

	inline DrmType& operator^=(DrmType& type_a, DrmType type_b) {
		return (DrmType&)((int&)(type_a) ^= (int)(type_b));
	}

	inline TrackType operator&(TrackType type_a, TrackType type_b) {
		return static_cast<TrackType>(static_cast<int>(type_a) & static_cast<int>(type_b));
	}

	inline TrackType operator|(TrackType type_a, TrackType type_b) {
		return static_cast<TrackType>(static_cast<int>(type_a) | static_cast<int>(type_b));
	}

	inline TrackType operator^(TrackType type_a, TrackType type_b) {
		return static_cast<TrackType>(static_cast<int>(type_a) ^ static_cast<int>(type_b));
	}

	inline TrackType& operator&=(TrackType& type_a, TrackType type_b) {
		return (TrackType&)((int&)(type_a) &= (int)(type_b));
	}

	inline TrackType& operator|=(TrackType& type_a, TrackType type_b) {
		return (TrackType&)((int&)(type_a) |= (int)(type_b));
	}

	inline TrackType& operator^=(TrackType& type_a, TrackType type_b) {
		return (TrackType&)((int&)(type_a) ^= (int)(type_b));
	}

	struct MultiDrmInfo
	{
		std::string trackType;
		std::string key;
		std::string keyId;
		std::string iv;
		std::string periodIndex;
		std::string widevinePSSH;
		std::string widevinePSSHpayload;
		std::string playreadyPSSH;
		std::string playreadyPSSHpayload;
		std::string fairplayHlsSignalingData;
		std::string fairplayHlsKeyUri;
		std::string ncgCek;
		std::string ncgHlsKeyUri;
	};

	struct ContentPackagingInfo
	{
		std::string contentId;
		std::vector<MultiDrmInfo> multiDrmInfos;
	};

	class CpixClient
	{
	private:
		std::string _kmsUrl;
		std::string _encToken;

		int _lastRequestStatus;
		std::string _lastRequestRowData;
		std::string _lastResponseRowData;
	
		std::map<std::string, std::string> _keyMap;

		std::string GetRequestData(std::string contentId, DrmType drmType, EncryptionScheme encryptionScheme, TrackType trackType, long periodIndex);
		ContentPackagingInfo ParseResponse(const std::string& responseBody);

	public:
		/**
		* Constructor.
		*
		* @param strKmsURL				PallyCon KMS Server URL
		* @param strEncToken			PallyCon API authentication token
		*/
		CpixClient(std::string strKmsURL, std::string strEncToken);
		~CpixClient();


		/**
		* Get the last request status value.
		*/
		int GetLastRequestStatus() const {
			return _lastRequestStatus;
		}

		/**
		* Get the last request data.
		*/
		std::string GetLastRequestRowData() const {
			return _lastRequestRowData;
		}

		/**
		* Get the last response data.
		*/
		std::string GetLastResponseRowData() const {
			return _lastResponseRowData;
		}

		/**
		* Receive packaging information from PallyCon KMS server.
		*
		* @param contentId				Content id
		* @param drmType				DRM type. (e.g. WIDEVINE|PLAYREADY|FAIRPLAY)
		* @param encryptionScheme		Encryption scheme. (e.g. CENC, CBCS, etc)
		* @param trackType				Track type for multi-key packaging. (e.g. SD|HD|AUDIO)
		*								For single-key packaging, it should be ALL_TRACKS.
		* @param periodIndex			Period index for key rotation. 
										Setting a value greater than 0 enables key rotation.
		*/
		ContentPackagingInfo GetContentKeyInfoFromPallyConKMS(const std::string contentId, DrmType drmType, EncryptionScheme encryptionScheme = CENC, TrackType trackType = ALL_TRACKS, long periodIndex = 0);
	};
}

