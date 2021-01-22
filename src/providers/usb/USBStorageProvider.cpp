/* @@@LICENSE
 *
 * Copyright (c) 2020 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#include <functional>
#include <future>
#include <pbnjson.hpp>
#include "USBStorageProvider.h"
#include "SA_Common.h"
#include "SAFLunaService.h"

#define SAF_USB_ATTACH_METHOD  "luna://com.webos.service.pdm/getAttachedStorageDeviceList"
#define SAF_USB_WRITE_Q_METHOD "luna://com.webos.service.pdm/isWritableDrive "
#define SAF_USB_SPACE_METHOD   "luna://com.webos.service.pdm/getSpaceInfo"
#define SAF_USB_FORMAT_METHOD  "luna://com.webos.service.pdm/format"
#define SAF_USB_EJECT_METHOD   "luna://com.webos.service.pdm/eject"

USBStorageProvider::USBStorageProvider() : mQuit(false)
{
	mUsbDispatcherThread = std::thread(std::bind(&USBStorageProvider::dispatchHandler, this));
	mUsbDispatcherThread.detach();
}

USBStorageProvider::~USBStorageProvider()
{
	mQuit = true;
	if (mUsbDispatcherThread.joinable())
	{
		mUsbDispatcherThread.join();
	}
}

ReturnValue USBStorageProvider::attachCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue USBStorageProvider::authenticateCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue USBStorageProvider::listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit)
{
    return nullptr;
}

ReturnValue USBStorageProvider::getProperties(AuthParam authParam)
{
    return nullptr;
}

ReturnValue USBStorageProvider::copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue USBStorageProvider::move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue USBStorageProvider::remove(AuthParam authParam, string storageId, string path)
{
    return nullptr;
}

ReturnValue USBStorageProvider::eject(string storageId)
{
    return nullptr;
}

ReturnValue USBStorageProvider::format(string storageId, string fileSystem, string volumeLabel)
{
    return nullptr;
}

void USBStorageProvider::testMethod(std::shared_ptr<RequestData> data)
{
	LOG_DEBUG_SAF("%s", __FUNCTION__);
	std::string uri = "luna://com.webos.service.pdm/getAttachedStorageDeviceList";
	std::string payload = R"({"subscribe": true})";
	LSError lserror;
	(void)LSErrorInit(&lserror);
	ReqContext *ctxPtr = new ReqContext();
	ctxPtr->ctx = this;
	ctxPtr->reqData = std::move(data);

	pbnjson::JValue nextReqArray = pbnjson::Array();
#if 0
	pbnjson::JValue nextObj1 = pbnjson::Object();
	nextObj1.put("uri", uri);
	nextObj1.put("payload", payload);
	//nextObj1.put("params", payload);
	nextReqArray.append(nextObj1);
#endif
	pbnjson::JValue nextObj2 = pbnjson::Object();
	std::string uri2 = "luna://com.webos.service.pdm/isWritableDrive";
	std::string payload2 = R"({"driveName": "sdg1"})";
	nextObj2.put("uri", uri2);
	nextObj2.put("payload", payload2);
	//nextObj2.put("params", payload);
	nextReqArray.append(nextObj2);

	ctxPtr->reqData->params.put("nextReq", nextReqArray);
	LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
				USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
	//ToDo: Handle Error Scenarios
}

void USBStorageProvider::listStoragesMethod(std::shared_ptr<RequestData> data)
{
	LOG_DEBUG_SAF("%s", __FUNCTION__);
	std::string uri = "luna://com.webos.service.pdm/getAttachedStorageDeviceList";
	std::string payload = R"({"subscribe": true})";
	LSError lserror;
	(void)LSErrorInit(&lserror);
	ReqContext *ctxPtr = new ReqContext();
	ctxPtr->ctx = this;
	ctxPtr->reqData = std::move(data);

	pbnjson::JValue nextReqArray = pbnjson::Array();
	pbnjson::JValue nextObj = pbnjson::Object();

	nextObj.put("uri", uri);
	nextObj.put("payload", payload);
	nextReqArray.append(nextObj);

	LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
				USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
}

void USBStorageProvider::addRequest(std::shared_ptr<RequestData>& reqData)
{
	mUSBReqQueue.push_back(std::move(reqData));
	mCondVar.notify_one();
}

bool USBStorageProvider::onReply(LSHandle *sh, LSMessage *message , void *ctx)
{
	LOG_DEBUG_SAF("%s: [%s]", __FUNCTION__, LSMessageGetPayload(message));
	LSError lserror;
	(void)LSErrorInit(&lserror);
	LSCallCancel(sh, NULL, &lserror);
	ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
	USBStorageProvider* self = static_cast<USBStorageProvider*>(ctxPtr->ctx);
	pbnjson::JSchema parseSchema = pbnjson::JSchema::AllSchema();
	pbnjson::JDomParser parser;
	std::string payload = LSMessageGetPayload(message);
	if (parser.parse(payload, parseSchema))
	{
		pbnjson::JValue root = parser.getDom();
		if (!ctxPtr->reqData->params.hasKey("response"))
		{
			pbnjson::JValue respArray = pbnjson::Array();
			ctxPtr->reqData->params.put("response", respArray);
		}
		pbnjson::JValue respArray = ctxPtr->reqData->params["response"];
		respArray.append(root);
		ctxPtr->reqData->params.put("response", respArray);
		if (ctxPtr->reqData->params.hasKey("nextReq") &&
			ctxPtr->reqData->params["nextReq"].isArray() &&
			(ctxPtr->reqData->params["nextReq"].arraySize() > 0))
		{
			std::string uri = ctxPtr->reqData->params["nextReq"][0]["uri"].asString();
			std::string payload = ctxPtr->reqData->params["nextReq"][0]["payload"].asString();
			pbnjson::JValue nextReqArr = pbnjson::Array();
			for (int i=1; i< ctxPtr->reqData->params["nextReq"].arraySize(); ++i)
				nextReqArr.append(ctxPtr->reqData->params["nextReq"][i]);
			ctxPtr->reqData->params.put("nextReq", nextReqArr);
			LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
				USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
		}
		else
			ctxPtr->reqData->cb(ctxPtr->reqData->params["response"], ctxPtr->reqData->subs);
	}
	return true;
}

void USBStorageProvider::handleRequests(std::shared_ptr<RequestData> reqData)
{
	LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
	switch(reqData->methodType)
	{
		case MethodType::TEST_METHOD:
		{
			LOG_DEBUG_SAF("%s : MethodType::TEST_METHOD", __FUNCTION__);
			auto fut = std::async(std::launch::async, [this, reqData]() { return this->testMethod(reqData); });
			(void)fut;
		}
		case MethodType::LIST_STORAGES_METHOD:
		{
			LOG_DEBUG_SAF("%s : MethodType::LIST_STORAGES_METHOD", __FUNCTION__);
			auto fut = std::async(std::launch::async, [this, reqData]() { return this->listStoragesMethod(reqData); });
			(void)fut;
		}
		break;
		default:
		break;
	}
}

void USBStorageProvider::dispatchHandler()
{
	LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::unique_lock < std::mutex > lock(mMutex);
	do {
		mCondVar.wait(lock, [this] {
			return (mUSBReqQueue.size() || mQuit);
		});
		LOG_DEBUG_SAF("Dispatch notif received : %d, mQuit: %d", mUSBReqQueue.size(), mQuit);
		if (mUSBReqQueue.size() && !mQuit)
		{
			lock.unlock();
			handleRequests(std::move(mUSBReqQueue.front()));
			mUSBReqQueue.erase(mUSBReqQueue.begin());
			lock.lock();
		}
	} while (!mQuit);
}

