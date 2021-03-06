#ifndef CEF_MY_V8_HANDLER_
#define CEF_MY_V8_HANDLER_

#include <string>
#include <thread>

#include "datastore.h"
#include "include/cef_urlrequest.h"
#include "include/cef_v8.h"
#include "include/cef_values.h"
#include "xmlparser.h"

class MyV8Handler : public CefV8Handler, public CefURLRequestClient {
 public:
  MyV8Handler() {}

  virtual void OnRequestComplete(CefRefPtr<CefURLRequest> request) OVERRIDE {
    CefRefPtr<CefResponse> response = request->GetResponse();
    int status = response->GetStatus();

    if (status == 200) {
      CefV8ValueList args;
      CefRefPtr<CefV8Value> retval;

      // Parse TTC response HTML
      PriceCheck item;
      bool exists = ParseTTCPriceCheck(download_data_, &item);

      if (item.name.empty()) {
        // item not found, return undefined
        args.push_back(CefV8Value::CreateUndefined());
        callback_function_->ExecuteFunctionWithContext(callback_context_, NULL,
                                                       args);
      } else {
        // item found, return item details and info on whether its a new
        // item or an update of existing one.
        callback_context_->Enter();
        CefRefPtr<CefV8Value> ret = CefV8Value::CreateObject(NULL, NULL);
        ret->SetValue("exists", CefV8Value::CreateBool(exists),
                      V8_PROPERTY_ATTRIBUTE_NONE);
        ret->SetValue("name", CefV8Value::CreateString(item.name),
                      V8_PROPERTY_ATTRIBUTE_NONE);
        ret->SetValue("min_suggest", CefV8Value::CreateDouble(item.min_suggest),
                      V8_PROPERTY_ATTRIBUTE_NONE);
        ret->SetValue("max_suggest", CefV8Value::CreateDouble(item.max_suggest),
                      V8_PROPERTY_ATTRIBUTE_NONE);
        ret->SetValue("img_src", CefV8Value::CreateString(item.img_src),
                      V8_PROPERTY_ATTRIBUTE_NONE);
        callback_context_->Exit();
        args.push_back(ret);
        callback_function_->ExecuteFunctionWithContext(callback_context_, NULL,
                                                       args);
      }

      download_data_.clear();
    } else {
      CefV8ValueList args;
      args.push_back(CefV8Value::CreateString(download_data_));
      callback_function_->ExecuteFunctionWithContext(callback_context_, NULL,
                                                     args);
    }
  }

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) OVERRIDE {
    if (name == "ttc_price_check") {
      if (arguments.size() == 2) {
        CefRefPtr<CefV8Value> arg = arguments[0];
        CefRefPtr<CefV8Value> func = arguments[1];
        if ((arg.get()->IsValid() && arg.get()->IsString()) &&
            (func.get()->IsValid() && func.get()->IsFunction())) {
          std::string value = arg.get()->GetStringValue();

          // save callback JS function and context
          this->callback_function_ = func;
          this->callback_context_ = CefV8Context::GetCurrentContext();

          CefRefPtr<CefRequest> request = CefRequest::Create();
          // Populate |request| as shown above...
          request->SetURL(
              "https://eu.tamrieltradecentre.com/pc/Trade/"
              "SearchResult?ItemID=&SearchType=PriceCheck&ItemNamePattern=" +
              value);
          request->SetMethod("GET");

          // Start the request. MyRequestClient callbacks will be executed
          // asynchronously.
          CefRefPtr<CefURLRequest> url_request =
              CefURLRequest::Create(request, this, nullptr);
        }
        // Set up the CefRequest object.
      }
      // Return my string value.
      // retval = CefV8Value::CreateString("My Value!");
      return true;
    } else if (name == "remove_watched_item") {
      if (arguments.size() == 1) {
        CefRefPtr<CefV8Value> arg = arguments[0];
        if ((arg.get()->IsValid() && arg.get()->IsString())) {
          std::string item_name = arg.get()->GetStringValue();
          DataStore::RemoveItem(item_name);
          return true;
        }
      }
    } else if (name == "ttc_find_deals") {
      CefRefPtr<CefV8Value> func = arguments[0];

      if (func.get()->IsValid() && func.get()->IsFunction()) {
        DataStore* datastore = DataStore::getInstance();
        std::map<std::string, PriceCheck> items = datastore->GetItemsSnapshot();
        if (items.size() == 0) {
          return true;
        }

        UpdateHandler* handler = new UpdateHandler(
            CefV8Context::GetCurrentContext(), func, (int)items.size());

        std::map<std::string, PriceCheck>::iterator it;
        for (it = items.begin(); it != items.end(); it++) {
          // Send a request for this item
          std::string query;
          CefRefPtr<CefRequest> request = CefRequest::Create();
          query =
              "https://eu.tamrieltradecentre.com/pc/Trade/"
              "SearchResult?ItemID=&ItemNamePattern=" +
              it->second.name + "&SortBy=Price&Order=asc&PriceMax=" +
              std::to_string((int)(it->second.min_suggest) - 1);

          request->SetURL(query);
          request->SetMethod("GET");

          CefRefPtr<CefURLRequest> url_request =
              CefURLRequest::Create(request, handler, nullptr);
        }

        // execute DoItemSearch in a new thread
        // std::thread t(&UpdateHandler::DoItemSearch, handler, NULL);
        // t.join();
        return true;
      }
    }

    // Function does not exist.
    return false;
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

 private:
  std::string download_data_;
  CefRefPtr<CefV8Context> callback_context_;
  CefRefPtr<CefV8Value> callback_function_;
  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(MyV8Handler);
};

#endif  // CEF_MY_V8_HANDLER_