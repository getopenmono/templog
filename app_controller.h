#ifndef app_controller_h
#define app_controller_h

#include <mono.h>
#include "graph_view.h"
#include "settings_scene.h"
#include "internet.h"

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
    network::HttpPostClient client;
    
    mono::ui::TextLabelView timeLbl;
    ScheduledTask tmpTask, uploadTask;
    
    mono::ui::ButtonView settingsBtn;
    
    SettingsScene settingScn;
    Internet internet;
    
    AppController();

    float getTemp(bool firstRun = false);
    void getTempTask();
    
    void uploadTempAsync();
    void uploadTemp();
    void uploadTempReady();
    
    void showSettings();
    void showApp();
    
    void updateClock();

    void fillTempFilter(int temp);
    
    void monoWakeFromReset();
    void monoWillGotoSleep();
    void monoWakeFromSleep();

    uint16_t postBodyLength();
    void postBody(char *data);
    void tempUploadCompletion(network::INetworkRequest::CompletionEvent *);

};

#endif /* app_controller_h */
