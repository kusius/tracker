#ifndef CEF_UPDATE_V8_HANDLER_
#define CEF_UPDATE_V8_HANDLER_
#include <string>

#include "datastore.h"
#include "include/base/cef_bind.h"
#include "include/cef_urlrequest.h"
#include "include/cef_v8.h"
#include "include/cef_values.h"
#include "include/wrapper/cef_closure_task.h"
#include "xmlparser.h"

class UpdateHandler : public CefV8Handler, public CefURLRequestClient {
 public:
  UpdateHandler(CefRefPtr<CefV8Context> context,
                CefRefPtr<CefV8Value> func,
                int pending_requests)
      : callback_context_(context),
        callback_function_(func),
        pending_requests(pending_requests) {
    acuired_deals.clear();
  };

  void SetCallbackContext(CefRefPtr<CefV8Context> context) {
    callback_context_ = context;
  }
  void SetCallbackFunction(CefRefPtr<CefV8Value> func) {
    callback_function_ = func;
  }

  virtual void OnRequestComplete(CefRefPtr<CefURLRequest> request) OVERRIDE {
    CefRefPtr<CefResponse> response = request->GetResponse();
    int status = response->GetStatus();

    if (status == 200) {
      ItemDealVec item_deals;
      ParseItemDeals(download_data_, &item_deals);
      download_data_.clear();

      m_.lock();
      pending_requests--;
      acuired_deals.insert(acuired_deals.end(), item_deals.begin(),
                           item_deals.end());
      m_.unlock();

      if (pending_requests == 0) {
        SendDataToUI();
        pending_requests = -1;
      }
    }
  }

  void SendDataToUI() {
    if (callback_function_->IsFunction() && callback_context_->IsValid()) {
      if (pending_requests != 0) {
        LOG(ERROR)
            << "Attempt to notify UI with non-zero pending requests!! Abort";
        return;
      }
      callback_context_->Enter();
      // return item list
      CefV8ValueList args;
      CefRefPtr<CefV8Value> ret =
          CefV8Value::CreateArray((int)acuired_deals.size());
      CefRefPtr<CefV8Value> item;
      for (unsigned int i = 0; i < acuired_deals.size(); i++) {
        item = CefV8Value::CreateObject(NULL, NULL);
        item->SetValue("name", CefV8Value::CreateString(acuired_deals[i].name),
                       V8_PROPERTY_ATTRIBUTE_NONE);
        item->SetValue("location",
                       CefV8Value::CreateString(acuired_deals[i].location),
                       V8_PROPERTY_ATTRIBUTE_NONE);
        item->SetValue("trader",
                       CefV8Value::CreateString(acuired_deals[i].trader),
                       V8_PROPERTY_ATTRIBUTE_NONE);
        item->SetValue("mins_elapsed",
                       CefV8Value::CreateInt(acuired_deals[i].mins_elapsed),
                       V8_PROPERTY_ATTRIBUTE_NONE);
        item->SetValue("price",
                       CefV8Value::CreateDouble(acuired_deals[i].price),
                       V8_PROPERTY_ATTRIBUTE_NONE);
        item->SetValue("trade_id",
                       CefV8Value::CreateUInt(acuired_deals[i].trade_id),
                       V8_PROPERTY_ATTRIBUTE_NONE);
        ret->SetValue(i, item);
      }

      args.push_back(ret);
      callback_function_->ExecuteFunctionWithContext(callback_context_, NULL,
                                                     args);

      callback_context_->Exit();
      return;
    }
  }

  void OnUploadProgress(CefRefPtr<CefURLRequest> request,
                        int64 current,
                        int64 total) OVERRIDE {}

  void OnDownloadProgress(CefRefPtr<CefURLRequest> request,
                          int64 current,
                          int64 total) OVERRIDE {}

  void OnDownloadData(CefRefPtr<CefURLRequest> request,
                      const void* data,
                      size_t data_length) OVERRIDE {
    download_data_ += std::string(static_cast<const char*>(data), data_length);
  }

  bool GetAuthCredentials(bool isProxy,
                          const CefString& host,
                          int port,
                          const CefString& realm,
                          const CefString& scheme,
                          CefRefPtr<CefAuthCallback> callback) OVERRIDE {
    return false;  // Not handled.
  }
  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) OVERRIDE {
    if (name == "register") {
      if (arguments.size() == 1 && arguments[0]->IsFunction()) {
        callback_function_ = arguments[0];
        callback_context_ = CefV8Context::GetCurrentContext();
        return true;
      }
    }
    return true;
  }

 private:
  CefRefPtr<CefV8Context> callback_context_;
  CefRefPtr<CefV8Value> callback_function_;
  std::string download_data_;
  std::recursive_mutex m_;
  int pending_requests;  // This should be locked/atomic!!
  ItemDealVec acuired_deals;
  IMPLEMENT_REFCOUNTING(UpdateHandler);
};

#endif  // CEF_UPDATE_V8_HANDLER_