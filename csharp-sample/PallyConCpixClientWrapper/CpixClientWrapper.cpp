#include <iostream>
#include <msclr/marshal_cppstd.h>

#include "CpixClientWrapper.h"

namespace PallyCon {
	PallyCon::CpixClientWrapper::CpixClientWrapper(String^ strKmsURL, String^ strKmsToken) 
		: _cpixClient(new pallycon::CpixClient(msclr::interop::marshal_as<std::string>(strKmsURL)
		, msclr::interop::marshal_as<std::string>(strKmsToken)))
	{
	}

	PallyCon::CpixClientWrapper::~CpixClientWrapper()
	{
		delete _cpixClient;
		_cpixClient = nullptr;
	}

	int CpixClientWrapper::GetLastRequestStatus()
	{
		return _cpixClient->GetLastRequestStatus();
	}

	String^ CpixClientWrapper::GetLastRequestRowData()
	{
		return gcnew String(_cpixClient->GetLastRequestRowData().c_str());
	}

	String^ CpixClientWrapper::GetLastResponseRowData()
	{
		return gcnew String(_cpixClient->GetLastResponseRowData().c_str());
	}

	ContentPackagingInfo PallyCon::CpixClientWrapper::GetContentKeyInfoFromPallyConKMS(String^ cid, DrmType drmType, EncryptionScheme encryptionScheme, TrackType trackType, long periodIndex)
	{
		ContentPackagingInfo packInfos;
		try {
			pallycon::ContentPackagingInfo contentPackInfo = _cpixClient->GetContentKeyInfoFromPallyConKMS(msclr::interop::marshal_as<std::string>(cid)
				, static_cast<pallycon::DrmType>(drmType), static_cast<pallycon::EncryptionScheme>(encryptionScheme), static_cast<pallycon::TrackType>(trackType), periodIndex);

			packInfos.ContentId = gcnew String(contentPackInfo.contentId.c_str());
			packInfos.DrmInfos = gcnew List<MultiDrmInfo>;

			for (auto multiDrmInfo : contentPackInfo.multiDrmInfos)
			{
				MultiDrmInfo drmInfo;
				drmInfo.TrackType = gcnew String(multiDrmInfo.trackType.c_str());
				drmInfo.Key = gcnew String(multiDrmInfo.key.c_str());
				drmInfo.KeyId = gcnew String(multiDrmInfo.keyId.c_str());
				drmInfo.Iv = gcnew String(multiDrmInfo.iv.c_str());
				drmInfo.PeriodIndex = gcnew String(multiDrmInfo.periodIndex.c_str());
				drmInfo.WidevinePSSH = gcnew String(multiDrmInfo.widevinePSSH.c_str());
				drmInfo.WidevinePSSHpayload = gcnew String(multiDrmInfo.widevinePSSHpayload.c_str());
				drmInfo.PlayReadyPSSH = gcnew String(multiDrmInfo.playreadyPSSH.c_str());
				drmInfo.PlayReadyPSSHpayload = gcnew String(multiDrmInfo.playreadyPSSHpayload.c_str());
				drmInfo.FairplayHlsSignalingData = gcnew String(multiDrmInfo.fairplayHlsSignalingData.c_str());
				drmInfo.FairplayHlsKeyUri = gcnew String(multiDrmInfo.fairplayHlsKeyUri.c_str());
				drmInfo.NcgCek = gcnew String(multiDrmInfo.ncgCek.c_str());
				drmInfo.NcgHlsKeyUri = gcnew String(multiDrmInfo.ncgHlsKeyUri.c_str());

				packInfos.DrmInfos->Add(drmInfo);
			}
		}
		catch (pallycon::CpixClientException& e)
		{
			std::string errMsg = "An error has occurred in the CPIX Client module : \n";
			errMsg.append(e.what());
			throw gcnew Exception(gcnew String(errMsg.c_str()));
		}
		catch (std::exception& e)
		{
			throw gcnew Exception(gcnew String(e.what()));
		}

		return packInfos;
	}
}