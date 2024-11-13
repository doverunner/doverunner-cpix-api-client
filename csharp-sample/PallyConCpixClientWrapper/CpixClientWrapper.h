#pragma once

#include "CpixClient.h"

using namespace System;
using namespace System::Collections::Generic;

namespace PallyCon {
	public enum class EncryptionScheme {
		CENC,
		CBC1,
		CENS,
		CBCS
	};

	public enum class DrmType {
		WIDEVINE = (1 << 0),
		PLAYREADY = (1 << 1),
		FAIRPLAY = (1 << 2),
		NCG = (1 << 3),
		HLS_NCG = (1 << 4),
		WISEPLAY = (1 << 5),
	};

	public enum class TrackType {
		ALL_TRACKS,
		AUDIO = (1 << 0),
		SD = (1 << 1),
		HD = (1 << 2),
		UHD1 = (1 << 3),
		UHD2 = (1 << 4)
	};

	public value struct MultiDrmInfo {
		String^ TrackType;
		String^ Key;
		String^ KeyId;
		String^ Iv;
		String^ PeriodIndex;
		String^ WidevinePSSH;
		String^ WidevinePSSHpayload;
		String^ WidevineHlsSignalingDataMaster;
		String^ WidevineHlsSignalingDataMedia;
		String^ PlayReadyPSSH;
		String^ PlayReadyPSSHpayload;
		String^ PlayReadySmoothStreamingData;
		String^ PlayReadyHlsSignalingDataMaster;
		String^ PlayReadyHlsSignalingDataMedia;
		String^ FairplayHlsKeyUri;
		String^ FairplayHlsSignalingDataMaster;
		String^ FairplayHlsSignalingDataMedia;
		String^ NcgCek;
		String^ NcgHlsKeyUri;
		String^ WiseplayPSSH;
		String^ WiseplayPSSHpayload;
		String^ WiseplayHlsSignalingDataMaster;
		String^ WiseplayHlsSignalingDataMedia;	
	};

	public value struct ContentPackagingInfo {
		String^ ContentId;
		List<MultiDrmInfo>^ DrmInfos;
	};

	public ref class CpixClientWrapper
	{
	private:
		pallycon::CpixClient* _cpixClient;

	public:
		CpixClientWrapper(String^ kmsUrl);
		virtual ~CpixClientWrapper();

		int GetLastRequestStatus();
		String^ GetLastRequestRowData();
		String^ GetLastResponseRowData();

		ContentPackagingInfo GetContentKeyInfoFromPallyConKMS(String^ cid, DrmType drmType, EncryptionScheme encryptionScheme, TrackType trackType, long periodIndex);
	};
}
