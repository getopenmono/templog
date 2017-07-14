
#ifndef settings_scene_h
#define settings_scene_h

#include <mono.h>
#include <mbed.h>

using namespace mono;
using namespace mono::geo;

class SettingsScene : public mono::ui::SceneController {
    
protected:
    
    mono::ui::TextLabelView timeScaleLbl, ssidLbl;
    mono::ui::ButtonView hrs4, hrs8, hrs24, hrs48, connectBtn, backBtn;
    
    void hrs4Call(); void hrs8Call(); void hrs24Call(); void hrs48Call();
    void cnctCall();
    
    mbed::FunctionPointer dismissHandler;
    
public:
    
    mbed::FunctionPointerArg1<void, int> timescaleHandler;
    mbed::FunctionPointer connectHandler;
    
    SettingsScene();
    
    void didShow(const SceneController&);
    void didHide(const SceneController&);
    
    void requestDismiss();
    
    template <typename Context>
    void setDimissCallback(Context *cnxt, void(Context::*memptr)(void))
    {
        dismissHandler.attach<Context>(cnxt, memptr);
    }
    
};

#endif /* settings_scene_h */
