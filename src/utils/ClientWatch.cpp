// @@@LICENSE
//
//      Copyright (c) 2018-2022 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.
//
// LICENSE@@@

#include <SAFLog.h>
#include "ClientWatch.h"

namespace LSUtils
{

ClientWatch::ClientWatch(LSHandle *handle, LSMessage *message, ClientWatchStatusCallback callback) :
    mHandle(handle),
    mMessage(message),
    mCookie(0),
    mCallback(callback),
    mNotificationTimeout(0)
{
    if (!mMessage)
        return;

    LSMessageRef(mMessage);
    startWatching();
}

ClientWatch::~ClientWatch()
{
    LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "[mNotificationTimeout:%u]%s():%d",mNotificationTimeout,__FUNCTION__, __LINE__);
    // Don't send any pending notifications as that will fail
    if (mNotificationTimeout)
        g_source_remove(mNotificationTimeout);

    if (mCookie)
    {
        LS::Error error;

        if (!LSCancelServerStatus(mHandle, mCookie, error.get()))
            error.log(PmLogGetLibContext(), "LS_FAILED_TO_UNREG_SRV_STAT");
    }

    LSCallCancelNotificationRemove(mHandle, &ClientWatch::clientCanceledCallback, this, NULL);
}

bool ClientWatch::serverStatusCallback(LSHandle *, const char *, bool connected, void *context)
{
    ClientWatch *watch = static_cast<ClientWatch*>(context);
    if (nullptr == watch)
        return false;

    if (connected)
        return true;

    watch->notifyClientDisconnected();

    return true;
}

bool ClientWatch::clientCanceledCallback(LSHandle *, const char *uniqueToken, void *context)
{
    ClientWatch *watch = static_cast<ClientWatch*>(context);
    if (nullptr == watch)
        return false;

    watch->notifyClientCanceled(uniqueToken);

    return true;
}

void ClientWatch::startWatching()
{
    if (!mMessage)
        return;

    const char *serviceName = LSMessageGetSender(mMessage);

    LS::Error error;
    if (!LSRegisterServerStatusEx(mHandle, serviceName, &ClientWatch::serverStatusCallback,
                                  this, &mCookie, error.get()))
        throw error;

    if (!LSCallCancelNotificationAdd(mHandle, &ClientWatch::clientCanceledCallback, this, error.get()))
        throw error;
}

gboolean ClientWatch::sendClientDroppedNotification(gpointer user_data)
{
    LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "%s():%d",__FUNCTION__, __LINE__);
    if (nullptr == user_data)
        return FALSE;

    ClientWatch *watch = static_cast<ClientWatch*>(user_data);
    watch->mNotificationTimeout = 0;

    return FALSE;
}

void ClientWatch::triggerClientDroppedNotification()
{
    LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "[mNotificationTimeout:%u]%s():%d",mNotificationTimeout,__FUNCTION__, __LINE__);
    if (mNotificationTimeout)
        return;

    // We have to offload the actual callback here as otherwise we risk
    // a deadlock when someone tries to destroy us while stilling being
    // in the callback from ls2
    mNotificationTimeout = g_timeout_add(0, &ClientWatch::sendClientDroppedNotification, this);
    LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "[mNotificationTimeout:%u]%s():%d",mNotificationTimeout,__FUNCTION__, __LINE__);
}

void ClientWatch::notifyClientDisconnected()
{
    LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "%s():%d",__FUNCTION__, __LINE__);
    triggerClientDroppedNotification();
}

void ClientWatch::notifyClientCanceled(const char *clientToken)
{
    if (!mMessage)
        return;

    const char *messageToken = LSMessageGetUniqueToken(mMessage);

    if(messageToken && clientToken && !g_strcmp0(messageToken, clientToken)){
        LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "%s():%d",__FUNCTION__, __LINE__);
        triggerClientDroppedNotification();
    }

    if (mMessage)
        LSMessageUnref(mMessage);
}

} // namespace LS

