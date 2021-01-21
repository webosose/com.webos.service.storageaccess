// @@@LICENSE
//
//      Copyright (c) 2018 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#ifndef CLIENTWATCH_H
#define CLIENTWATCH_H

#include <string>
#include <luna-service2/lunaservice.hpp>

namespace LSUtils
{

typedef std::function<void(void)> ClientWatchStatusCallback;

class ClientWatch
{
public:
	ClientWatch(LSHandle *handle, LSMessage *message, ClientWatchStatusCallback callback);
	ClientWatch(const ClientWatch &other) = delete;
	~ClientWatch();

	LSMessage *getMessage() const { return mMessage; }
	void setCallback(ClientWatchStatusCallback callback) { mCallback = callback; }

private:
	LSHandle *mHandle;
	LSMessage *mMessage;
	void *mCookie;
	ClientWatchStatusCallback mCallback;
	guint mNotificationTimeout;

	void startWatching();
	void cleanup();
	void triggerClientDroppedNotification();

	void notifyClientDisconnected();
	void notifyClientCanceled(const char *clientToken);

	static gboolean sendClientDroppedNotification(gpointer user_data);
	static bool serverStatusCallback(LSHandle *, const char *, bool connected, void *context);
	static bool clientCanceledCallback(LSHandle *, const char *uniqueToken, void *context);
};

} // namespace LS

#endif // CLIENTWATCH_H
