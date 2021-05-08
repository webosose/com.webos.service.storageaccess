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

#include <stdlib.h>
#include <string.h>
#include <SAFLog.h>
#include "SAFErrors.h"
#include "UpnpDiscover.h"

UpnpDiscover& UpnpDiscover::getInstance()
{
    static UpnpDiscover obj;
    return obj;
}

UpnpDiscover::UpnpDiscover() : mInterface(NULL), mTarget(NULL),
    mMsgType(NULL), mTimeout(2), mRescanInterval(0), mClient(NULL),
    mBrowser(NULL), mInitThreadPtr(nullptr), mScanCompFlag(false)
{
    mDiscover.main_loop = NULL;
    mDiscover.client = NULL;
    mDiscover.browser = NULL;
    mScannedDevs.clear();
    mScannedDevs.resize(0);
    init();
}

UpnpDiscover::~UpnpDiscover()
{
    if (mDiscover.main_loop)
        g_main_loop_unref (mDiscover.main_loop);
    if (mDiscover.browser)
        g_object_unref (mDiscover.browser);
    if (mDiscover.client)
        g_object_unref (mDiscover.client);
    if (mInterface)
        g_free (mInterface);
    if (mTarget)
        g_free (mTarget);
    if (mMsgType)
        g_free (mMsgType);
    mDiscover.main_loop = NULL;
    mDiscover.client = NULL;
    mDiscover.browser = NULL;
    mInterface = mTarget = mMsgType = NULL;
}

void UpnpDiscover::init()
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    mEntries.clear();
    mEntries.push_back({ "interface", 'i', 0, G_OPTION_ARG_STRING, &mInterface,
        "Network INTERFACE to use", "INTERFACE" });
    mEntries.push_back({ "target", 't', 0, G_OPTION_ARG_STRING, &mTarget,
        "SSDP TARGET to search for (default: ssdp:all)", "TARGET" });
    mEntries.push_back({ "timeout", 'n', 0, G_OPTION_ARG_INT, &mTimeout,
        "TIME in seconds to wait for replies before exiting", "TIME" });
    mEntries.push_back({ "rescan-interval", 'r', 0, G_OPTION_ARG_INT, &mRescanInterval,
        "TIME in seconds to wait before sending another discovery request", "TIME" });
    mEntries.push_back({ "message-type", 'm', 0, G_OPTION_ARG_STRING, &mMsgType,
        "TYPE of message (available,unavailable,all)", "TYPE" });
    mEntries.push_back({});
#if 0
    if (nullptr == mInitThreadPtr)
    {
        mInitThreadPtr = std::unique_ptr<std::thread>(
            new std::thread(&UpnpDiscover::startScan, this));
        mInitThreadPtr->detach();
    }
#endif
}

void UpnpDiscover::deinit()
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    mScanCompFlag = true;
}

void UpnpDiscover::updateDeviceInfo(std::string msg, bool addFlag)
{
    LOG_DEBUG_SAF("%s: %s, %d", __FUNCTION__, msg.c_str(), addFlag);
    if (addFlag)
    {
        mScannedDevs.push_back(msg);
    }
    else
    {
        mScannedDevs.clear();
    }
}

void UpnpDiscover::on_resource_unavailable(GSSDPResourceBrowser*,
    const char* usn)
{
    std::string usnStr = usn;
    if (usnStr.find(UPNP_CONTENT_DIR) != std::string::npos)
    {
        LOG_DEBUG_SAF("%s resource unavailable %s", __FUNCTION__, usnStr.c_str());
        UpnpDiscover::getInstance().updateDeviceInfo (usnStr, false);
    }
}

void UpnpDiscover::on_resource_available(GSSDPResourceBrowser* browser,
    const char* usn, GList* locations)
{
    GList *l;
    std::string usnStr = usn;
    if (usnStr.find(UPNP_CONTENT_DIR) != std::string::npos)
    {
        LOG_DEBUG_SAF("%s: USN: %s", __FUNCTION__, usnStr.c_str());
        for (l = locations; l; l = l->next)
        {
            std::string location = (char *)l->data;
            if (location.find("description.xml") != std::string::npos)
            {
                LOG_DEBUG_SAF("%s:Location: %s", __FUNCTION__, location.c_str());
                UpnpDiscover::getInstance().updateDeviceInfo (location);
            }
        }
    }
}

gboolean UpnpDiscover::on_force_rescan_timeout(GSSDPDiscover *discover)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    gssdp_resource_browser_set_active(discover->browser, FALSE);
    gssdp_resource_browser_set_active (discover->browser, TRUE);
    return TRUE;
}

void UpnpDiscover::startScan()
{
    if (mScanCompFlag) return;
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    mScannedDevs.clear();
    gssdp_resource_browser_set_active (mDiscover.browser, FALSE);
    mScanCompFlag = true;
#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init ();
#endif
    //GOptionContext *context = g_option_context_new ("- discover devices using SSDP");
    //g_option_context_add_main_entries (context, mEntries.data(), NULL);
    //g_option_context_free (context);
    GError *error = NULL;
    if (mDiscover.client == NULL)
    {
        mDiscover.client = gssdp_client_new (mInterface, &error);
        if (error != NULL)
        {
            LOG_DEBUG_SAF("%s: Interface %s", __FUNCTION__,
                gssdp_client_get_interface (mDiscover.client));
            return;
        }
    }
    if (mTarget){
        if (mDiscover.browser == NULL)
            mDiscover.browser = gssdp_resource_browser_new(mDiscover.client, mTarget);
    }
    else
    {
        if (mDiscover.browser == NULL)
            mDiscover.browser = gssdp_resource_browser_new(mDiscover.client, "ssdp:all");
    }
    if (mMsgType == NULL)
    {
        mMsgType = g_strdup ("available");
    }
    else if (strncmp (mMsgType, "available", 9) != 0 &&
        strncmp (mMsgType, "all", 3) != 0 &&
        strncmp (mMsgType, "unavailable", 11) != 0)
    {
        LOG_DEBUG_SAF("%s: Invalid message type: %s", __FUNCTION__, mMsgType);
        return;
    }
    if (strncmp (mMsgType, "available", 9) == 0 ||
        strncmp (mMsgType, "all", 3) == 0)
    {
        g_signal_connect (mDiscover.browser, "resource-available",
            G_CALLBACK (UpnpDiscover::on_resource_available), &mDiscover);
    }
    if (strncmp (mMsgType, "unavailable", 11) == 0 ||
        strncmp (mMsgType, "all", 3) == 0)
    {
        g_signal_connect (mDiscover.browser, "resource-unavailable",
            G_CALLBACK (UpnpDiscover::on_resource_unavailable), &mDiscover);
    }
    LOG_DEBUG_SAF("%s: Message type: %s", __FUNCTION__, mMsgType);
    if (mDiscover.main_loop == NULL)
        mDiscover.main_loop = g_main_loop_new (NULL, FALSE);
    gssdp_resource_browser_set_active (mDiscover.browser, TRUE);
    if (mTimeout > 0)
    {
        g_timeout_add_seconds (mTimeout, 
            (GSourceFunc) g_main_loop_quit, mDiscover.main_loop);
    }
    if (mRescanInterval > 0 && (mRescanInterval < mTimeout || mTimeout == 0))
    {
        g_timeout_add_seconds (mRescanInterval, 
            (GSourceFunc) on_force_rescan_timeout, &mDiscover);
    }
    g_main_loop_run (mDiscover.main_loop);
    if (mDiscover.browser)
    {
        g_object_unref (mDiscover.browser);
        mDiscover.browser = NULL;
    }
    if (mDiscover.client)
    {
        g_object_unref (mDiscover.client);
        mDiscover.client = NULL;
    }
    mScanCompFlag = false;
    LOG_DEBUG_SAF("%s: Exiting SSDP discovery loop", __FUNCTION__);
}

void UpnpDiscover::stopScan()
{
    gssdp_resource_browser_set_active(mDiscover.browser, FALSE);
    //deinit();
}

std::vector<std::string>& UpnpDiscover::getScannedDevices()
{
    LOG_DEBUG_SAF("%s size: %d", __FUNCTION__, mScannedDevs.size());
    while (mScanCompFlag != false)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return mScannedDevs;
}

