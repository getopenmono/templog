
#ifndef settings_scene_h
#define settings_scene_h

#include <mono.h>
#include <mbed.h>

using namespace mono;
using namespace mono::geo;

class SettingsScene : public mono::ui::View {
    
protected:
    
    mono::ui::TextLabelView timeScaleLbl, ssidLbl;
    mono::ui::ButtonView hrs4, hrs8, hrs24, hrs48, connectBtn;
    
    void hrs4Call(); void hrs8Call(); void hrs24Call(); void hrs48Call();
    void cnctCall();
    
    
public:
    
    mbed::FunctionPointerArg1<void, int> timescaleHandler;
    mbed::FunctionPointer connectHandler, dismissHandler;
    
    SettingsScene();
    
    void show();
    void hide();
    
    void repaint();
    
};

#endif /* settings_scene_h */
