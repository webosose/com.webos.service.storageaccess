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

#ifndef __UPNP_DISCOVER_H__
#define __UPNP_DISCOVER_H__

#include <vector>
#include <thread>

#include <glib.h>
#include <libgssdp/gssdp.h>

#define UPNP_CONTENT_DIR "urn:schemas-upnp-org:service:ContentDirectory:1"

typedef struct _GSSDPDiscover {
    GMainLoop *main_loop;
    GSSDPClient *client;
    GSSDPResourceBrowser *browser;
}GSSDPDiscover;

class UpnpDiscover
{
public:
	static UpnpDiscover& getInstance();
	void startScan();
	void stopScan();
	void updateDeviceInfo(std::string msg, bool addFlag=true);
	std::vector<std::string>& getScannedDevices();
private:
	UpnpDiscover();
	UpnpDiscover& operator = (const UpnpDiscover&) = default;
	~UpnpDiscover();
	void init();
	void deinit();
	static void on_resource_unavailable(GSSDPResourceBrowser*, const char*);
	static void on_resource_available(GSSDPResourceBrowser*, const char*, GList*);
	static gboolean on_force_rescan_timeout(GSSDPDiscover*);
	std::unique_ptr<std::thread> mInitThreadPtr;
	char *mInterface;	
	char *mTarget;
	char *mMsgType;
	int mTimeout;
	int mRescanInterval;
	GSSDPClient *mClient;
	GSSDPResourceBrowser *mBrowser;
	GSSDPDiscover mDiscover;
	bool mScanCompFlag;
	std::vector<GOptionEntry> mEntries;
	std::vector<std::string> mScannedDevs;
};


#endif /*__UPNP_DISCOVER_H__*/

