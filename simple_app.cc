// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "simple_app.h"

#include <string>

#include "datastore.h"
#include "helpers_win.h"
#include "include/base/cef_bind.h"
#include "include/base/cef_logging.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "simple_handler.h"
#include "update_handler.h"
#include "v8_handler.h"

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
 public:
  explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {}

  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
    browser_view_ = nullptr;
  }

  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

  CefSize GetPreferredSize(CefRefPtr<CefView> view) OVERRIDE {
    return CefSize(800, 800);
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

class SimpleBrowserViewDelegate : public CefBrowserViewDelegate {
 public:
  SimpleBrowserViewDelegate() {}

  bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view,
                                 CefRefPtr<CefBrowserView> popup_browser_view,
                                 bool is_devtools) OVERRIDE {
    // Create a new top-level Window for the popup. It will show itself after
    // creation.
    CefWindow::CreateTopLevelWindow(
        new SimpleWindowDelegate(popup_browser_view));

    // We created the Window.
    return true;
  }

 private:
  IMPLEMENT_REFCOUNTING(SimpleBrowserViewDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleBrowserViewDelegate);
};

}  // namespace

SimpleApp::SimpleApp() {}

// CefRenderProcessHandler implementation
void SimpleApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context) {
  // Retrieve the context's window object.
  CefRefPtr<CefV8Value> object = context->GetGlobal();

  // Create an instance of my CefV8Handler object.
  CefRefPtr<CefV8Handler> handler = new MyV8Handler();

  CefRefPtr<CefV8Value> func =
      CefV8Value::CreateFunction("ttc_price_check", handler);
  object->SetValue("ttc_price_check", func, V8_PROPERTY_ATTRIBUTE_NONE);

  func = CefV8Value::CreateFunction("confirm_watched_item", handler);
  object->SetValue("confirm_watched_item", func, V8_PROPERTY_ATTRIBUTE_NONE);

  func = CefV8Value::CreateFunction("remove_watched_item", handler);
  object->SetValue("remove_watched_item", func, V8_PROPERTY_ATTRIBUTE_NONE);

  func = CefV8Value::CreateFunction("ttc_find_deals", handler);
  object->SetValue("ttc_find_deals", func, V8_PROPERTY_ATTRIBUTE_NONE);

  CefRefPtr<CefFrame> m_frame = browser->GetMainFrame();
  m_frame->ExecuteJavaScript(
      "var intervalID = setInterval(window.GlobalFuncs.DoSearch(), "
      "update_interval);",
      m_frame->GetURL(), 0);
  // Register our search function to execute after a delay on the render thread
  // Repost ourselves for the next update
  // CefRefPtr<UpdateHandler> instance = new UpdateHandler();
  // func = CefV8Value::CreateFunction("register", instance);
  // object->SetValue("register", func, V8_PROPERTY_ATTRIBUTE_NONE);

  // CefRefPtr<CefV8Handler> instance = new UpdateHandler();
  // CefPostDelayedTask(TID_RENDERER,
  //                 base::Bind(&UpdateHandler::DoItemSearch, instance, NULL),
  //               DEFAULT_UPDATE_DELAY);
  return;
}

void SimpleApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

#if defined(OS_WIN) || defined(OS_LINUX)
  // Create the browser using the Views framework if "--use-views" is specified
  // via the command-line. Otherwise, create the browser using the native
  // platform framework. The Views framework is currently only supported on
  // Windows and Linux.
  const bool use_views = command_line->HasSwitch("use-views");
#else
  const bool use_views = false;
#endif

  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views));

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;
  std::string url;

  // Check if a "--url=" value was provided via the command-line. If so, use
  // that instead of the default URL.
  url = command_line->GetSwitchValue("url");
  if (url.empty()) {
    char* buffer = new char[512];
    GerWorkingDirectory(buffer, 512);
    url = CefString(std::string(buffer) + std::string("/assets/app.html"));
    delete[] buffer;
  }

  if (use_views) {
    // Create the BrowserView.
    CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
        handler, url, browser_settings, nullptr, nullptr,
        new SimpleBrowserViewDelegate());

    // Create the Window. It will show itself after creation.
    CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
  } else {
    // Information used when creating the native window.
    CefWindowInfo window_info;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsPopup(NULL, "TTC Tracker");
#endif
    // IMPORTANT(George): Set width and height after SetAsPopup as they are
    // garbage since we called with a NULL HWND (windows)
    window_info.width = 960;
    window_info.height = 920;

    // Create the first browser window.
    CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings,
                                  nullptr, nullptr);
  }
}
