
#ifndef settings_scene_h
#define settings_scene_h

#include <mono.h>

using namespace mono;
using namespace mono::geo;

class SettingsScene : public mono::ui::View {
    
    mono::ui::TextLabelView timeScaleLbl, ssidLbl;
    mono::ui::ButtonView hrs4, hrs8, hrs24, hrs48, connectBtn;
    
public:
    
    SettingsScene();
    
    void show();
    void hide();
    
    void repaint();
    
};

#endif /* settings_scene_h */
