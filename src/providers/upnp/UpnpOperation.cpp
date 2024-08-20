/* @@@LICENSE
 *
 * Copyright (c) 2021 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#include <SAFLog.h>
#include "SAFErrors.h"
#include "UpnpOperation.h"

UpnpOperation& UpnpOperation::getInstance()
{
    static UpnpOperation obj;
    return obj;
}

UpnpOperation::UpnpOperation()
{
    init();
}

UpnpOperation::~UpnpOperation()
{
}

void UpnpOperation::init()
{
    printf("%s", __FUNCTION__);
}

void UpnpOperation::deinit()
{
    printf("%s", __FUNCTION__);
}

int UpnpOperation::getNextDirId(int pId, std::string nextDir)
{
    int id = -1;
    OC::Bridging::CurlClient curlClient;
    curlClient.setMethodType(OC::Bridging::CurlClient::CurlMethod::GET);
    curlClient.setURL(mControlUrl);
    curlClient.addRequestHeader(OC::Bridging::CURL_HEADER_ACCEPT_JSON);
    curlClient.setUpnpXmlData(pId);
    std::string resXml = curlClient.getUpnpXml();
    printf("resXml 1 %s", resXml.c_str());
    curlClient.setRequestBody(resXml);
    std::vector<std::string> reqHeaders;
    reqHeaders.push_back(OC::Bridging::CURL_UPNP_CONTENT_TYPE_XML);
    reqHeaders.push_back(OC::Bridging::CURL_UPNP_SOAP_ACTION);
    curlClient.setRequestHeaders(reqHeaders);
    if (0 == curlClient.send())
    {
        resXml = curlClient.getResponseBody();
        printf("resXml 2 %s", resXml.c_str());
        XMLHandler xmlHandObj(resXml);
        resXml = xmlHandObj.getValue("Result");
        printf("resXml 3 %s", resXml.c_str());
        xmlHandObj.setXmlContent(std::move(resXml));
        auto dirs = xmlHandObj.getCurDirDetails(pId);
        for (const auto& dir : dirs)
        {
            if (dir.title == nextDir)
            {
                id = dir.id;
                break;
            }
        }
    }
    return id;
}

int UpnpOperation::getContainerId(std::string url, std::string path)
{
    printf("%s: url: %s, path: %s", __FUNCTION__, url.c_str(), path.c_str());
    mControlUrl.clear();
    int contId = -1;
    OC::Bridging::CurlClient curlClient;
    curlClient.setMethodType(OC::Bridging::CurlClient::CurlMethod::GET);
    curlClient.setURL(url);
    curlClient.addRequestHeader(OC::Bridging::CURL_HEADER_ACCEPT_JSON);
    if (curlClient.send() != 0)
    {
        printf("%s: Error in sending curl for %s", __FUNCTION__, url.c_str());
        return contId;
    }
    std::string temp = curlClient.getResponseBody();
    printf("temp1:%s\n",temp.c_str());
    mXmlHandObj.setXmlContent (temp);
    std::string controlUrl = mXmlHandObj.getValueUnderSameParent("controlURL",
        "serviceType", "service:ContentDirectory:1");
    temp = url.substr(0, url.rfind("/"));
    printf("controlUrl %s______temp2:%s", controlUrl.c_str(),temp.c_str());
    curlClient.setURL(temp + controlUrl);
    curlClient.setMethodType(OC::Bridging::CurlClient::CurlMethod::POST);
    mControlUrl = temp + controlUrl;
    std::vector<std::string> reqHeaders;
    reqHeaders.push_back(OC::Bridging::CURL_UPNP_CONTENT_TYPE_XML);
    reqHeaders.push_back(OC::Bridging::CURL_UPNP_SOAP_ACTION);
    curlClient.setRequestHeaders(reqHeaders);
    mDirPathNames.clear();
    temp = path;
    auto pos = temp.find("/");
    while (pos != std::string::npos)
    {
        temp = temp.substr(pos+1);
        pos = temp.find("/");
        std::string val = temp.substr(0, pos);
        printf("val:%s",val.c_str());
        if (!val.empty())
            mDirPathNames.push_back(val);
        else
            pos += 1;
    }
    int id = 0;
    for (auto & dirName : mDirPathNames)
    {
        printf("mDirPathNames ");
        id = getNextDirId(id, dirName);
        contId = id;
        printf("dirName %s   ---- ID : %d", dirName.c_str(),id);
        if (id < 0)
        {
            printf("%s: Invalid path: %s", __FUNCTION__, path.c_str());
            break;
        }
    }
    return contId;
}

std::vector<DirDetails> UpnpOperation::listDirContents(int id)
{
    OC::Bridging::CurlClient curlClient;
    curlClient.setMethodType(OC::Bridging::CurlClient::CurlMethod::GET);
    curlClient.setURL(mControlUrl);
    curlClient.addRequestHeader(OC::Bridging::CURL_HEADER_ACCEPT_JSON);
    curlClient.setUpnpXmlData(id);
    std::string temp = curlClient.getUpnpXml();
    curlClient.setRequestBody(temp);
    std::vector<std::string> reqHeaders;
    reqHeaders.push_back(OC::Bridging::CURL_UPNP_CONTENT_TYPE_XML);
    reqHeaders.push_back(OC::Bridging::CURL_UPNP_SOAP_ACTION);
    curlClient.setRequestHeaders(reqHeaders);
    curlClient.send();
    temp = curlClient.getResponseBody();
    mXmlHandObj.setXmlContent(temp);
    temp = mXmlHandObj.getValue("Result");
    mXmlHandObj.setXmlContent(std::move(temp));
    return mXmlHandObj.getCurDirDetails(id);
}

