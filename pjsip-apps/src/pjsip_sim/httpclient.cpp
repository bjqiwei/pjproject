#include "httpclient.h"

#include <iostream>

#define CURL_STATICLIB

extern "C"{
#include "curl/curl.h"
}

using namespace std;

HttpClient::HttpClient(void) : m_bDebug(false), prepot(-1) {}

HttpClient::~HttpClient(void) {}
// 1.0.11.x_906
static int OnDebug(CURL *, curl_infotype itype, char *pData, size_t size,
                   void *) {
  if (itype == CURLINFO_TEXT) {
    printf("[TEXT]%s\n", pData);
  } else if (itype == CURLINFO_HEADER_IN) {
    printf("[HEADER_IN]%s\n", pData);
  } else if (itype == CURLINFO_HEADER_OUT) {
    printf("[HEADER_OUT]%s\n", pData);
  } else if (itype == CURLINFO_DATA_IN) {
    printf("[DATA_IN]%s\n", pData);
  } else if (itype == CURLINFO_DATA_OUT) {
    printf("[DATA_OUT]%s\n", pData);
  }
  return 0;
}

static size_t OnWriteData(void *buffer, size_t size, size_t nmemb,
                          void *lpVoid) {
  std::string *str = (std::string *)lpVoid;
  if (NULL == str || NULL == buffer) {
    return -1;
  }

  char *pData = (char *)buffer;
  str->append(pData, size * nmemb);
  return nmemb;
}
/*  libcurl write callback function */
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;

  //在这里可以把下载到的数据以追加的方式写入文件
  // FILE* fp = NULL;
  // fopen_s(&fp, "c:\\test.dat", "ab+");//一定要有a,
  // 否则前面写入的内容就会被覆盖了 size_t nWrite = fwrite(ptr, nSize, nmemb,
  // fp); fclose(fp); return nWrite;
}

#if 0
int EasyCurl::progress_callback(void *pParam, double dltotal, double dlnow, double ultotal, double ulnow)
{
	EasyCurl* pThis = (EasyCurl*)pParam;
	int nPos = (int)((dlnow / dltotal) * 100);
	//
	if (pThis->m_pHttpCallback)
	{
		pThis->m_pHttpCallback->OnProgressCallback(nPos);
	}
	if (pThis->m_updateProgress)
	{
		pThis->m_updateProgress(nPos);
	}
	return 0;
}
#else
int HttpClient::progress_callback(void *pParam, double dltotal, double dlnow,
                                  double ultotal, double ulnow) {
  HttpClient *pThis = (HttpClient *)pParam;
  int nPos = (int)((dlnow / dltotal) * 100);
  if (nPos >= 0)
    if (nPos > pThis->prepot) std::cout << "下载进度：%" << nPos << std::endl;
  pThis->prepot = nPos;
  return 0;
}
#endif
//
int HttpClient::Post(const std::string &strUrl,
                          const std::string &strParam, std::string &strResponse,
                          const std::vector<std::string> &headers,
                          long &httpCode, const char *pCaPath) {
  CURLcode res;
  CURL *curl = curl_easy_init();
  if (NULL == curl) {
    return CURLE_FAILED_INIT;
  }
  //添加自定义包头
  struct curl_slist *headerlist = NULL; /*init to NULL is important*/
  for (const auto &h : headers) {
    headerlist = curl_slist_append(headerlist, h.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
  if (m_bDebug) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
  }
  curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strParam.c_str());
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  if (NULL == pCaPath) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
  } else {
    //缺省情况就是PEM，所以无需设置，另外支持DER
    // curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
    curl_easy_setopt(curl, CURLOPT_CAINFO, pCaPath);
  }
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
  res = curl_easy_perform(curl);
  if (CURLE_OK == res) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  }
  curl_slist_free_all(headerlist);
  curl_easy_cleanup(curl);
  return res;
}
//
int HttpClient::Get(const string &strUrl, string &strResponse,
                         const std::vector<std::string> &headers,
                         long &httpCode, const char *pCaPath) {
  CURLcode res;
  CURL *curl = curl_easy_init();
  if (NULL == curl) {
    return CURLE_FAILED_INIT;
  }
  //添加自定义包头
  struct curl_slist *headerlist = NULL; /*init to NULL is important*/
  for (const auto &h : headers) {
    headerlist = curl_slist_append(headerlist, h.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

  if (m_bDebug) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
  }
  curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  if (NULL == pCaPath) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,
                     false);  //设定为不验证证书和HOST
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
  } else {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
    curl_easy_setopt(curl, CURLOPT_CAINFO, pCaPath);
  }
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
  res = curl_easy_perform(curl);
  if (CURLE_OK == res) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  }
  curl_easy_cleanup(curl);
  return res;
}
//
int HttpClient::download_file(const std::string &strUrl,
                              const std::string &strFile) {
  //	FILE *fp;
  //	//调用curl_easy_init()函数得到 easy interface型指针
  //	CURL *curl = curl_easy_init();
  //	if (curl)
  //	{
  //		fopen_s(&fp, strFile.c_str(), "wb");
  //
  //		CURLcode res = curl_easy_setopt(curl, CURLOPT_URL,
  //strUrl.c_str()); 		if (res != CURLE_OK)
  //		{
  //			fclose(fp);
  //			curl_easy_cleanup(curl);
  //			return -1;
  //		}
  //
  //		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  //		if (res != CURLE_OK)
  //		{
  //			fclose(fp);
  //			curl_easy_cleanup(curl);
  //			return -1;
  //		}
  //
  //		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  //		if (res != CURLE_OK)
  //		{
  //			fclose(fp);
  //			curl_easy_cleanup(curl);
  //			return -1;
  //		}
  //		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
  //		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
  //progress_callback);//设置进度回调函数 		curl_easy_setopt(curl,
  //CURLOPT_PROGRESSDATA, this);
  //		//开始执行请求
  //		res = curl_easy_perform(curl);
  //		fclose(fp);
  //		/* Check for errors */
  //		if (res != CURLE_OK)
  //		{
  //			curl_easy_cleanup(curl);
  //			return -1;
  //		}
  //		curl_easy_cleanup(curl);//调用curl_easy_cleanup()释放内存
  //	}

  return 0;
}
//
void HttpClient::set_progress_function(ProgressFunction func) {
  m_updateProgress = func;
}
//
void HttpClient::set_progress_callback(IProgressCallback *pCallback) {
  m_pHttpCallback = pCallback;
}

//
void HttpClient::SetDebug(bool bDebug) { m_bDebug = bDebug; }

const char *HttpClient::strerror(int err) {
  return curl_easy_strerror((CURLcode)err);
}

size_t receive_data(void *buffer, size_t size, size_t nmemb, FILE *file) {
  size_t r_size = fwrite(buffer, size, nmemb, file);
  return r_size;
}

#if 0
int main() //解析页面并保存txt
{
	//char url[] = "http://www.baidu.com"; 
	char url[] = "https://www.alipay.com";
	char path[] = "D:\save_file.txt";
	FILE *file = fopen(path, "w");
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, receive_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
	cout << curl_easy_perform(handle);

	fclose(file);
	curl_easy_cleanup(handle);
	curl_global_cleanup();
	return 0;
}
//#else
int main()//根据url下载文件
{
	string url = "https://issuecdn.baidupcs.com/issue/netdisk/yunguanjia/BaiduNetdisk_ydsd6_7.2.1.1.exe";
	string strFile = "D:\\百度云盘.exe";
	EasyCurl* download = new EasyCurl;
	int ret = download->download_file(url,strFile);
	if (ret == 0)
	{
		std::cout << "文件下载完毕！" << std::endl;
	}
	else
	{
		std::cout << "error:" << ret << std::endl;
	}
	delete download;
	return ret;
}
#endif

#if 0
int main()
{
	string url = "https://www.alipay.com";
	string temp = "D:\\http_get.txt";
	FILE* file;
	fopen_s(&file, temp.c_str(),"ab+" );
	EasyCurl* httpget = new EasyCurl;
	int ret = httpget->http_get(url, temp, NULL);
	fclose(file);
	std::cout << "ret=" << ret << std::endl;
	std::cout << "temp = " << temp << std::endl;
	return ret;
}
#endif
