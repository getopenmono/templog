#ifndef app_controller_h
#define app_controller_h

#include <mono.h>
#include "graph_view.h"
#include "settings_scene.h"

//using namespace mono::ui;
using namespace mono;

class AppController : public mono::IApplication {
public:

    Timer clkTimer;
    mono::ui::TextLabelView tempLbl;
    GraphView graph;
    static const int filterLength = 3;
    int tempFilter[filterLength];
    unsigned int filterPosition;
    PowerSaver pwrsave;
    int secInterval;
    
    mono::ui::TextLabelView timeLbl;
    ScheduledTask tmpTask;
    
    mono::ui::ButtonView settingsBtn;
    
    SettingsScene settingScn;
    
    AppController();

    float getTemp(bool firstRun = false);
    void getTempTask();
    
    void showSettings();
    void showApp();
    
    void updateClock();

    void fillTempFilter(int temp);
    
    void monoWakeFromReset();
    void monoWillGotoSleep();
    void monoWakeFromSleep();

};

#endif /* app_controller_h */
