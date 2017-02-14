#ifndef app_controller_h
#define app_controller_h

#include <mono.h>
#include "graph_view.h"
#include "settings_scene.h"
#include "internet.h"
#include <io/hysteresis_trigger.h>
#include <url.h>

//using namespace mono::ui;
using namespace mono;

class Notification {
public:
    DateTime eventTime;
    String Title;
    String Message;
    bool isSent;
    
    Notification() : isSent(true) {}
    Notification(String title, String mesg)
    {
        eventTime = DateTime::now();
        Title = title;
        Message = mesg;
        isSent = false;
    }
};

class AppController : public mono::IApplication {
public:

    Timer clkTimer;
    mono::ui::TextLabelView tempLbl;
    GraphView graph;
    static const int filterLength = 3;
    static const int uploadIntervalMins = 2;
    int tempFilter[filterLength];
    unsigned int filterPosition;
    PowerSaver pwrsave;
    int secInterval, uploadTempIntervalSecs;
    network::HttpPostClient client;
    
    mono::ui::TextLabelView timeLbl, dataLabel;
    ScheduledTask tmpTask, uploadTask;
    
    mono::ui::ButtonView settingsBtn;
    
    SettingsScene settingScn;
    Internet internet;
    
    io::HysteresisTrigger levelTrigger;
    Notification levelNotice;
    bool shouldSendNotification;
    bool enableLevelNotifications;
    bool enableTempUploads;
    
    AppController();

    float getTemp(bool firstRun = false);
    void getTempTask();
    
    void uploadTempAsync();
    void uploadTemp();
    void uploadTempReady();
    
    void sendNotification(String title, String message);
    void _sendNotification();
    
    void showSettings();
    void showApp();
    
    void updateClock();
    
    void setLowTempState();
    void setHighTempState();

    void fillTempFilter(int temp);
    
    void monoWakeFromReset();
    void monoWillGotoSleep();
    void monoWakeFromSleep();

    uint16_t postBodyLength();
    void postBody(char *data);
    void tempUploadCompletion(network::INetworkRequest::CompletionEvent *);
    void networkError();
    
    uint16_t noticeBodyLength();
    void noticeBodyData(char *data);
    void noticeCompletion(network::INetworkRequest::CompletionEvent *);
    void noticeResponse(const network::HttpClient::HttpResponseData &);

};

#endif /* app_controller_h */
