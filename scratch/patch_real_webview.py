import re

with open('src/Singularity/Foreign/Web/RealWebView.cpp', 'r') as f:
    content = f.read()

# 1. Add domMirror to script message handlers
setup_handlers = """        // Set up bridge for communication
        WebViewBridge* bridge = [[WebViewBridge alloc] init];
        bridge.webView = _webView;
        bridge.config = _webConfig;
        bridge.userContentController = _userContentController;
        
        // Set up message handler
        __weak RealWebView* weakSelf = this;
        bridge.messageHandler = ^(NSString* message) {
            if (weakSelf) {
                weakSelf->_handleWebMessage([message UTF8String]);
            }
        };
        
        // Register handlers
        [_userContentController addScriptMessageHandler:bridge name:@"earthcall"];
        [_userContentController addScriptMessageHandler:bridge name:@"domMirror"];"""

content = re.sub(r'        // Set up bridge for communication.*?\[_userContentController addScriptMessageHandler:bridge name:@"earthcall"\];', setup_handlers, content, flags=re.DOTALL)

# 2. Fix _jsHandlers dispatch in _handleWebMessage
# Since dom_mirror.js sends { "type": "snapshot", "payload": ... }, we should dispatch based on the name.
# Wait, domMirror messages are handled by `registerJavaScriptHandler("domMirror", ...)`
# We need to dispatch to ALL _jsHandlers or pass it.
# Actually, dom_mirror.js sends messages, but in `_handleWebMessage`, how do we know it came from "domMirror"?
# We don't. The message handler in Objective-C receives `WKScriptMessage*`, which has `message.name`.
# So we should pass `message.name`!
