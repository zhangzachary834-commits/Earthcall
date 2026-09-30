import re

with open('src/Singularity/Foreign/Web/RealWebView.cpp', 'r') as f:
    content = f.read()

# Replace the end of _handleWebMessage to dispatch to _jsHandlers
dispatch_code = """    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse web message: " << e.what() << std::endl;
    }
    
    // Also call the original message handler for backward compatibility
    if (_messageHandler) {
        _messageHandler(message);
    }
    
    // Dispatch to registered JS handlers
    for (const auto& pair : _jsHandlers) {
        pair.second(message);
    }
}"""

content = re.sub(r'    \} catch \(const std::exception& e\) \{.*?_messageHandler\(message\);\n    \}\n\}', dispatch_code, content, flags=re.DOTALL)

with open('src/Singularity/Foreign/Web/RealWebView.cpp', 'w') as f:
    f.write(content)
