using System.Text.Json;

namespace PallyCon
{
    class Program
    {
        public class JsonPackagingData
        {
            public string? content_id { get; set; }
            public IList<Dictionary<string, object>>? content_key_list { get; set; }
        }
        public static string HexToBase64(string strHex)
        {
            try
            {
                var bytes = new byte[strHex.Length / 2];
                for (var i = 0; i < bytes.Length; i++)
                {
                    bytes[i] = System.Convert.ToByte(strHex.Substring(i * 2, 2), 16);
                }

                return System.Convert.ToBase64String(bytes);
            }
            catch(Exception)
            {
                throw;
            }
        }
        public static string Base64ToHex(string strBase64)
        {
            try
            {
                var bytes = Convert.FromBase64String(strBase64);
                return BitConverter.ToString(bytes).Replace("-", "").ToLower();
            }
            catch (Exception)
            {
                throw;
            }
        }
        static string MakeJsonStringFromData(ContentPackagingInfo packInfo) 
        {
            string contentId = packInfo.ContentId;
            List<Dictionary<string, object>> listDrmInfos = new List<Dictionary<string, object>>();

            foreach(var drmInfo in packInfo.DrmInfos)
            {
                var dictDrmInfo = new Dictionary<string, object>();
                
                if(packInfo.DrmInfos.Count > 1)
                    dictDrmInfo["track_type"] = drmInfo.TrackType;

                dictDrmInfo["key_id_hex"] = drmInfo.KeyId;
                dictDrmInfo["key_id_b64"] = HexToBase64(drmInfo.KeyId);
                dictDrmInfo["key_hex"] = Base64ToHex(drmInfo.Key);
                dictDrmInfo["key_b64"] = drmInfo.Key;
                dictDrmInfo["iv_hex"] = Base64ToHex(drmInfo.Iv);
                dictDrmInfo["iv_b64"] = drmInfo.Iv;
                dictDrmInfo["widevine"] = new Dictionary<string, string> { ["pssh"] = drmInfo.WidevinePSSH, ["pssh_payload_only"] = drmInfo.WidevinePSSHpayload };
                dictDrmInfo["playready"] = new Dictionary<string, string> { ["pssh"] = drmInfo.PlayReadyPSSH, ["pssh_payload_only"] = drmInfo.PlayReadyPSSHpayload };
                dictDrmInfo["fairplay"] = new Dictionary<string, string> { ["key_uri"] = drmInfo.FairplayHlsKeyUri };

                listDrmInfos.Add(dictDrmInfo);
            }

            var jsonPackagingData = new JsonPackagingData
            {
                content_id = contentId,
                content_key_list = listDrmInfos
            };

            return JsonSerializer.Serialize(jsonPackagingData, new JsonSerializerOptions { WriteIndented = true });
        }
        static void Main(string[] args)
        {
            string kmsUrl = "https://kms.pallycon.com/v2/cpix/pallycon/getKey/";
            string encToken = ""; // PallyCon KMS token
            string contentId = ""; // Content id

            try
            {
                CpixClientWrapper pallyconCpixClientWrapper = new CpixClientWrapper(kmsUrl, encToken);
                ContentPackagingInfo contentPackagingInfo = pallyconCpixClientWrapper.GetContentKeyInfoFromPallyConKMS(
                    contentId, DrmType.WIDEVINE| DrmType.PLAYREADY|DrmType.FAIRPLAY, EncryptionScheme.CENC, TrackType.ALL_TRACKS);
                
                string jsonString = MakeJsonStringFromData(contentPackagingInfo);
                Console.WriteLine(jsonString);

                string fileName = contentPackagingInfo.ContentId + ".json";
                File.WriteAllText(fileName, jsonString);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }
    }
}