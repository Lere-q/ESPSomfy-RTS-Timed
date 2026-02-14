#ifndef WEBUI_H
#define WEBUI_H

#include <Arduino.h>

// ============================================
// SKYEO - Embedded Web UI
// Single Page Application in PROGMEM
// ============================================

class WebUI {
public:
    // Main HTML
    static const char* getIndexHTML();
    
    // CSS Styles
    static const char* getMainCSS();
    
    // JavaScript
    static const char* getMainJS();
    
    // Content length
    static size_t getIndexHTMLSize();
    static size_t getMainCSSSize();
    static size_t getMainJSSize();
};

#endif // WEBUI_H
