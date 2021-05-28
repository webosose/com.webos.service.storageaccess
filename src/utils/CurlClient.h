#ifndef _CURLCLIENT_H_
#define _CURLCLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <stdexcept>
#include "CurlErrorCode.h"
#include "CurlConstants.h"

namespace OC
{
    namespace Bridging
    {
        const char CURL_CONTENT_TYPE_JSON[] = "content-type: application/json";
        const char CURL_CONTENT_TYPE_URL_ENCODED[] = "content-type: application/x-www-form-urlencoded";
        const char CURL_HEADER_ACCEPT_JSON[] = "accept: application/json";
		const char CURL_UPNP_CONTENT_TYPE_XML[] = "content-type: text/xml";
		const char CURL_UPNP_CONTENT_ENC_UTF8[] = "charset=utf-8";
		const char CURL_UPNP_SOAP_ACTION[] = "soapaction: \"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"";
		const char CURL_UPNP_DEF_OBJ_ID[] = "0";
		const char CURL_UPNP_BROWSE_CHILD_FLAG[] = "BrowseDirectChildren";
		const char CURL_UPNP_BROWSE_METADATA_FLAG[] = "BrowseMetadata";
		const char CURL_UPNP_DEF_FILTER[] = "*";
		const char CURL_UPNP_DEF_START_INDEX[] = "0";
		const char CURL_UPNP_DEF_REQ_COUNT[] = "0";
		const char CURL_UPNP_DEF_SORT_CRITERIA[] = "";

        const long INVALID_RESPONSE_CODE = 0;

        class CurlClient
        {

            public:
                enum class CurlMethod
                {
                    GET, PUT, POST, DELETE, HEAD
                };

                virtual ~CurlClient() { }

                long getLastResponseCode()
                {
                    return m_lastResponseCode;
                }

                CurlClient()
                {
                    m_useSsl = CURLUSESSL_TRY;
                    m_lastResponseCode = INVALID_RESPONSE_CODE;

                }

                CurlClient(CurlMethod method, const std::string &url)
                {
                    if (url.empty())
                    {
                        throw "Curl method or url is empty";
                    }

                    m_method = getCurlMethodString(method);
                    m_url = url;
                    m_useSsl = CURLUSESSL_TRY;
                }

                CurlClient &setRequestHeaders(std::vector<std::string> &requestHeaders)
                {
                    m_requestHeaders = requestHeaders;
                    return *this;
                }

                CurlClient &addRequestHeader(const std::string &header)
                {
                    m_requestHeaders.push_back(header);
                    return *this;
                }

                CurlClient &setUserName(const std::string &userName)
                {
                    m_username = userName;
                    return *this;
                }

				CurlClient &setMethodType(const CurlMethod &method)
                {
                    m_method = getCurlMethodString(method);
                    return *this;
                }

				CurlClient &setURL(const std::string &url)
                {
                    m_url = url;
                    return *this;
                }

				CurlClient &setUpnpXmlData(int objId=0, std::string flag=CURL_UPNP_BROWSE_CHILD_FLAG,
                    std::string filter=CURL_UPNP_DEF_FILTER, int start=0, int count=0,
		    		std::string sort=CURL_UPNP_DEF_SORT_CRITERIA)
                {
				    std::string xml = "<?xml version=\"1.0\"?>\n";
				    xml += "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"";
		                    xml += "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n";
				    xml += " <s:Body>\n  <u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\n";
				    xml += "   <ObjectID>" + std::to_string(objId) + "</ObjectID>\n";
				    xml += "   <BrowseFlag>";
				    if (flag == CURL_UPNP_BROWSE_CHILD_FLAG)
		                        xml += CURL_UPNP_BROWSE_CHILD_FLAG;
				    else
		                        xml += CURL_UPNP_BROWSE_METADATA_FLAG;
				    xml += "</BrowseFlag>\n";
				    xml += "   <Filter>" + filter + "</Filter>\n";
				    xml += "   <StartingIndex>" + std::to_string(start) + "</StartingIndex>\n";
				    xml += "   <RequestedCount>" + std::to_string(count) + "</RequestedCount>\n";
				    xml += "   <SortCriteria>" + sort + "</SortCriteria>\n";
				    xml += "  </u:Browse>\n </s:Body>\n</s:Envelope>\n";
                    m_xml = xml;
                    return *this;
                }

                std::string getURL()
                {
                    return m_url;
                }

                std::string getUpnpXml()
                {
                    return m_xml;
                }

                CurlClient &setRequestBody(std::string &requestBody)
                {
                    m_requestBody = requestBody;
                    return *this;
                }

                CurlClient &setUseSSLOption(curl_usessl sslOption)
                {
                    m_useSsl = sslOption;
                    return *this;
                }

                int send()
                {
                    return doInternalRequest(m_url, m_method, m_requestHeaders, m_requestBody, m_username, m_outHeaders,
                                             m_response);
                }

                std::string getResponseBody()
                {
                    return m_response;
                }

                std::vector<std::string> getResponseHeaders()
                {
                    return m_outHeaders;
                }


            private:

                std::string getCurlMethodString(CurlMethod method)
                {
                    if (method == CurlMethod::GET)          return OC::PlatformCommands::GET;
                    else if (method == CurlMethod::PUT)     return OC::PlatformCommands::PUT;
                    else if (method == CurlMethod::POST)    return OC::PlatformCommands::POST;
                    else if (method == CurlMethod::DELETE)  return OC::PlatformCommands::DELETE;
                    else if (method == CurlMethod::HEAD)    return "HEAD";

                    else throw std::runtime_error("Invalid CurlMethod");
                }

                std::string m_url;
                std::string m_xml;
                std::string m_method;
                std::vector<std::string> m_requestHeaders;
                std::string m_requestBody;
                std::string m_username;
                std::string m_response;
                std::vector<std::string> m_outHeaders;

                /// Indicates whether to use CURLOPT_USE_SSL option in doInternalRequest.
                /// Curl default is no SSL (CURLUSESSL_NONE). Specify one of the other CURL SSL options
                /// (for example, CURLUSESSL_TRY) if you need to perform SSL transactions.
                curl_usessl m_useSsl;

                static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

                // Represents contiguous memory to hold a HTTP response.
                typedef struct _MemoryChunk
                {
                    _MemoryChunk() : memory(NULL), size(0)
                    {
                        memory = static_cast<char *>(malloc(1));
                    }
                    ~_MemoryChunk()
                    {
                        if(memory)
                        {
                            free(memory);
                            memory = NULL;
                        }
                    }

                    char *memory;
                    size_t size;

                } MemoryChunk;

                int decomposeHeader(const char *header, std::vector<std::string> &headers);


                int doInternalRequest(const std::string &url,
                                      const std::string &method,
                                      const std::vector<std::string> &inHeaders,
                                      const std::string &request,
                                      const std::string &username,
                                      std::vector<std::string> &outHeaders,
                                      std::string &response);

                long m_lastResponseCode;
        };
    } // namespace Bridging
}  // namespace OC
#endif // _CURLCLIENT_H_

