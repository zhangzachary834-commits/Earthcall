import re

with open('src/Singularity/Foreign/Web/RealWebView.cpp', 'r') as f:
    content = f.read()

# Replace WebViewBridge
interface = """@interface WebViewBridge : NSObject <WKScriptMessageHandler, WKNavigationDelegate>
@property (nonatomic, strong) WKWebView* webView;
@property (nonatomic, strong) WKWebViewConfiguration* config;
@property (nonatomic, strong) WKUserContentController* userContentController;
@property (nonatomic, copy) void (^messageHandler)(NSString* message);
@property (nonatomic, copy) void (^domMirrorHandler)(NSString* message);
@property (nonatomic, copy) void (^loadHandler)(BOOL loaded);
@end

@implementation WebViewBridge

- (void)userContentController:(WKUserContentController *)userContentController 
      didReceiveScriptMessage:(WKScriptMessage *)message {
    if ([message.name isEqualToString:@"domMirror"]) {
        if (self.domMirrorHandler) {
            self.domMirrorHandler(message.body);
        }
    } else {
        if (self.messageHandler) {
            self.messageHandler(message.body);
        }
    }
}"""

content = re.sub(r'@interface WebViewBridge.*?\}\n\}', interface, content, flags=re.DOTALL)

# In RealWebView::init(), add domMirrorHandler
setup = """        // Set up message handler
        __weak RealWebView* weakSelf = this;
        bridge.messageHandler = ^(NSString* message) {
            if (weakSelf) {
                weakSelf->_handleWebMessage([message UTF8String]);
            }
        };
        
        bridge.domMirrorHandler = ^(NSString* message) {
            if (weakSelf) {
                auto it = weakSelf->_jsHandlers.find("domMirror");
                if (it != weakSelf->_jsHandlers.end()) {
                    it->second([message UTF8String]);
                }
            }
        };"""
content = re.sub(r'        // Set up message handler.*?_handleWebMessage\(\[message UTF8String\]\);\n            \}\n        \};', setup, content, flags=re.DOTALL)

# Remove the dispatch we just added
content = re.sub(r'    // Dispatch to registered JS handlers\n    for \(const auto& pair : _jsHandlers\) \{\n        pair\.second\(message\);\n    \}\n', '', content)

with open('src/Singularity/Foreign/Web/RealWebView.cpp', 'w') as f:
    f.write(content)
