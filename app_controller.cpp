#include "app_controller.h"
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

const MonoFont *mono::ui::TextLabelView::StandardTextFont = 0;
const GFXfont *mono::ui::TextLabelView::StandardGfxFont = &FreeSans9pt7b;

const char* PushoverUrl = "https://api.pushover.net/1/messages.json";
const char* PushoverUserId = "";
const char* PushoverTokenId = "";
const char* PushoverDeviceId = "";

const char* GrapherUserId = "";
const char* GrapherContainerId = "";
const char* GrapherSeriesId = "";

AppController::AppController() :
    clkTimer(999),
    tempLbl(mono::geo::Rect(10,10,156,60),""),
    graph(109,109),
    timeLbl(mono::geo::Rect(0,65,176,25), ""),
    dataLabel(mono::geo::Rect(5,5,20,20), "d"),
    settingsBtn(mono::geo::Rect(150,5,176-151,30), "i"),
    levelTrigger(20,10)
{
    secInterval = 164;
    uploadTempIntervalSecs = 10;

    shouldSendNotification = false;
    enableLevelNotifications = true;
    enableTempUploads = true;
    
    tempLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    tempLbl.setFont(FreeSans18pt7b);
    tempLbl.setText(mono::display::CloudsColor);
    tempLbl.show();
    
    timeLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    timeLbl.setFont(FreeSans9pt7b);
    timeLbl.show();
    
    dataLabel.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    dataLabel.setFont(FreeSans9pt7b);
    dataLabel.setText(display::AlizarinColor);
    
    clkTimer.setCallback<AppController>(this, &AppController::updateClock);
    
    levelTrigger.setLowerTriggerCallback<AppController>(this, &AppController::setLowTempState);
    levelTrigger.setUpperTriggerCallback<AppController>(this, &AppController::setHighTempState);
    
#ifdef MONO_COMPILE_TIMESTAMP
    DateTime::setSystemDateTime(DateTime::fromISO8601(MONO_COMPILE_TIMESTAMP));
#endif
    timeLbl.setText(mono::DateTime::now().toTimeString());
    
    float temp = getTemp(true);
    graph.setSecsBetweenPoints(secInterval);
    graph.setNextPoint(temp);
    graph.show();
    
    settingsBtn.setClickCallback<AppController>(this, &AppController::showSettings);
    settingsBtn.show();
}

float AppController::getTemp(bool firstRun)
{
    int rawTemp = IApplicationContext::Instance->Temperature->ReadMilliCelcius();

    if (firstRun) {
        fillTempFilter(rawTemp);
    }
    else
    {
        tempFilter[filterPosition++] = rawTemp;
        if (filterPosition >= filterLength) { filterPosition = 0; }
    }

    int sum = 0;
    for (int i=0; i<filterLength; i++) {
        sum += tempFilter[i];
    }

    float tempC = (sum * 1.0) / (1000*filterLength);

    int temp = (int) tempC;
    tempLbl.setText(String::Format("%d.%d C", temp, (int)((tempC-temp)*10) ));
    
    if (enableLevelNotifications)
        levelTrigger.check(temp);
    
    return tempC;
}

void AppController::getTempTask()
{
    mono::power::IPowerSubSystem *power = IApplicationContext::Instance->PowerManager->PowerSystem;
    bool fenced = power->IsPowerFenced();
    
    if (fenced)
    {
        power->setPowerFence(false);
        wait_ms(16);
    }
    
    float temp = getTemp();
    graph.setNextPoint(temp);
    graph.scheduleRepaint();
    
    if (fenced)
        power->setPowerFence(true);
    
    updateClock();
    tmpTask.reschedule(DateTime::now().addSeconds(secInterval));
}

void AppController::updateClock()
{
    timeLbl.setText(mono::DateTime::now().toTimeString());
    getTemp();
}

void AppController::setLowTempState()
{
    printf("Templature low trigger!\r\n");
    sendNotification("Temperature too low", String::Format("Mono has measured a temperature of %s", tempLbl.Text().stringData));
}

void AppController::setHighTempState()
{
    printf("Templature normal trigger!\r\n");
    sendNotification("Temperature is good", String::Format("Normal temperature is established at %s", tempLbl.Text().stringData));
}

void AppController::fillTempFilter(int temp)
{
    for(int i=0; i<this->filterLength; i++)
    {
        tempFilter[i] = temp;
    }
    filterPosition = 0;
}

void AppController::uploadTempAsync()
{
    IApplicationContext::Instance->PowerManager->__shouldWakeUp = true;
    async<AppController>(this, &AppController::uploadTemp);
    
}
void AppController::uploadTemp()
{
#ifdef WLAN_SSID
    printf("Will connect and upload temp...\r\n");
    dataLabel.setText(display::AlizarinColor);
    dataLabel.show();
    
    internet.setConnectCallback<AppController>(this, &AppController::uploadTempReady);
    internet.setErrorCallback<AppController>(this, &AppController::networkError);
    
    if (!internet.isConnected())
    {
        internet.connect(WLAN_SSID, WLAN_PASS);
        pwrsave.deactivate();
    }
    else
        async<AppController>(this, &AppController::uploadTempReady);
#endif
}
void AppController::uploadTempReady()
{
    if (shouldSendNotification)
    {
        return _sendNotification();
    }
    
    printf("uploading temp...\r\n");
    String url = String::Format("http://grapher.openmono.com/autostamp/%s/%s/%s",
                                GrapherUserId, GrapherContainerId, GrapherSeriesId);
      client = network::HttpPostClient(url, "Content-Type: application/x-www-form-urlencoded\r\n");
    //client = network::HttpPostClient("http://kong.ptype.dk/autostamp/stoffera/templog/2", "Content-Type: application/x-www-form-urlencoded\r\n");
    client.setBodyLengthCallback<AppController>(this, &AppController::postBodyLength);
    client.setBodyDataCallback<AppController>(this, &AppController::postBody);
    client.setCompletionCallback<AppController>(this, &AppController::tempUploadCompletion);
    
    client.post();
}

void AppController::sendNotification(mono::String title, mono::String message)
{
    levelNotice = Notification(title, message);
    shouldSendNotification = true;
    uploadTempAsync();
}

void AppController::_sendNotification()
{
    printf("sending notification: %s...\r\n", levelNotice.Title());
    shouldSendNotification = false;
    
    client = network::HttpPostClient(PushoverUrl, "Content-Type: application/x-www-form-urlencoded\r\n");
    client.setBodyLengthCallback<AppController>(this, &AppController::noticeBodyLength);
    client.setBodyDataCallback<AppController>(this, &AppController::noticeBodyData);
    client.setCompletionCallback<AppController>(this, &AppController::noticeCompletion);
    client.post();
}

void AppController::showSettings()
{
    tempLbl.hide();
    timeLbl.hide();
    graph.hide();
    settingsBtn.hide();
    
    settingScn.show();
}

void AppController::showApp()
{
    settingScn.hide();
    
    tempLbl.show();
    timeLbl.show();
    graph.show();
    settingsBtn.show();
}

void AppController::monoWakeFromReset()
{
    clkTimer.Start();
    
    tmpTask = ScheduledTask(DateTime::now().addSeconds(secInterval));
    tmpTask.setRunInSleep(true);
    tmpTask.setTask<AppController>(this, &AppController::getTempTask);

    if (enableTempUploads)
    {
#ifdef WLAN_SSID
#ifdef WLAN_PASS
        uploadTask = ScheduledTask(DateTime::now().addSeconds(uploadTempIntervalSecs));
        uploadTask.setRunInSleep(true);
        uploadTask.setTask<AppController>(this, &AppController::uploadTempAsync);
#endif
#endif
    }
}

void AppController::monoWillGotoSleep()
{
    pwrsave.deactivate();
    IApplicationContext::Instance->DisplayController->setBrightness(0);
    dataLabel.setText(mono::ui::View::StandardBackgroundColor);
    dataLabel.scheduleRepaint();
    
    //secInterval = 10;
}

void AppController::monoWakeFromSleep()
{
    IApplicationContext::Instance->DisplayController->setBrightness(0);
    pwrsave.undim();
    
    showApp();
    tempLbl.scheduleRepaint();
    graph.scheduleRepaint();
    timeLbl.scheduleRepaint();

    secInterval = 164;
    graph.setSecsBetweenPoints(secInterval);
    getTemp();
    graph.scheduleRepaint();
    
    
}

uint16_t AppController::postBodyLength()
{
    int sum = 0;
    for (int i=0; i<filterLength; i++) {
        sum += tempFilter[i];
    }

    float tempC = (sum * 1.0) / (1000*filterLength);

    int temp = (int) tempC;
    uint16_t len = String::Format("%d.%d", temp, (int)((tempC-temp)*10) ).Length();
    printf("POST content-length is: %d\r\n",len);

    return len;
}

void AppController::postBody(char *data)
{
    int sum = 0;
    for (int i=0; i<filterLength; i++) {
        sum += tempFilter[i];
    }

    float tempC = (sum * 1.0) / (1000*filterLength);

    int temp = (int) tempC;
    String body = String::Format("%d.%d", temp, (int)((tempC-temp)*10) );
    printf("upload temp value: %s\r\n", body());

    memcpy(data, body.stringData, body.Length());
}

void AppController::tempUploadCompletion(network::INetworkRequest::CompletionEvent *)
{
    printf("Done! Reschedule upload temp...\r\n");
    dataLabel.setText(mono::ui::View::StandardBackgroundColor);
    dataLabel.scheduleRepaint();
    
    uploadTask.reschedule(DateTime::now().addMinutes(uploadIntervalMins));
    //async(IApplicationContext::EnterSleepMode);
    //pwrsave.startDimTimer();
    pwrsave.dim();
}

void AppController::networkError()
{
    dataLabel.setText(mono::ui::View::StandardBackgroundColor);
    dataLabel.scheduleRepaint();
    pwrsave.dim();
    printf("Network error!\r\n");
    uploadTask.reschedule(DateTime::now().addMinutes(uploadIntervalMins));
}

uint16_t AppController::noticeBodyLength()
{

    return snprintf(0, 0, "token=%s&user=%s&device=%s&title=%s&message=%s&url=&url_title=See+Graph",
                    PushoverTokenId, PushoverUserId, PushoverUserId,
                    levelNotice.Title(), levelNotice.Message()) +
           snprintf(0, 0, "http://grapher.openmono.com/data/%s/%s/%s?title=Templog&timeformat=MM-DD HH:mm&yaxis=℃",
                    GrapherUserId, GrapherContainerId, GrapherSeriesId);
}

void AppController::noticeBodyData(char *data)
{
    String url = String::Format("http://grapher.openmono.com/data/%s/%s/%s?title=Templog&timeformat=MM-DD HH:mm&yaxis=℃",
                                GrapherUserId, GrapherContainerId, GrapherSeriesId);
    snprintf(data, noticeBodyLength(), "token=%s&user=%s&device=%s&title=%s&message=%s&url=%s=&url_title=See+Graph",
             PushoverTokenId, PushoverUserId, PushoverUserId,
             levelNotice.Title(), levelNotice.Message(), url());
}

void AppController::noticeCompletion(network::INetworkRequest::CompletionEvent *)
{
    printf("Done sending notification!\r\n");
    dataLabel.setText(mono::ui::View::StandardBackgroundColor);
    dataLabel.scheduleRepaint();
    
    pwrsave.dim();
}

