#include "app_controller.h"
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <icons/upload-16.h>
#include <icons/settings-16.h>
#include <icons/thermometer-24.h>

const MonoFont *mono::ui::TextLabelView::StandardTextFont = 0;
const GFXfont *mono::ui::TextLabelView::StandardGfxFont = &FreeSans9pt7b;

const char* PushoverUrl = "http://api.pushover.net/1/messages.json";
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
    settingsBtn(mono::geo::Rect(150,5,176-151,30), " "),
    dataIcon(mono::geo::Point(5,13), upload16),
    settingsIcon(mono::geo::Point(155, 12), settings16),
    tempIcon(mono::geo::Point(10, 29), thermometer24),
    levelTrigger(15,5)
{
    secInterval = 164;
    uploadTempIntervalSecs = 20;

    shouldSendNotification = false;
    enableLevelNotifications = false;
    enableTempUploads = false;
    
    tempLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    tempLbl.setFont(FreeSans18pt7b);
    tempLbl.setText(mono::display::CloudsColor);
    
    timeLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    timeLbl.setFont(FreeSans9pt7b);
    
    clkTimer.setCallback<AppController>(this, &AppController::updateClock);
    
    levelTrigger.setLowerTriggerCallback<AppController>(this, &AppController::setLowTempState);
    levelTrigger.setUpperTriggerCallback<AppController>(this, &AppController::setHighTempState);
    
    settingScn.setDimissCallback(this, &AppController::showApp);
    
#ifdef MONO_COMPILE_TIMESTAMP
    DateTime::setSystemDateTime(DateTime::fromISO8601(MONO_COMPILE_TIMESTAMP));
#endif
    timeLbl.setText(mono::DateTime::now().toTimeString());
    
    float temp = getTemp(true);
    graph.setSecsBetweenPoints(secInterval);
    graph.setNextPoint(temp);
    
    dataIcon.setForeground(mono::ui::View::StandardBackgroundColor);
    
    settingsBtn.setClickCallback<AppController>(this, &AppController::showSettings);
    settingsBtn.setBorder(mono::ui::View::StandardBackgroundColor);
    
    mainScene.addView(tempLbl);
    mainScene.addView(tempIcon);
    mainScene.addView(timeLbl);
    mainScene.addView(dataIcon);
    mainScene.addView(graph);
    mainScene.addView(settingsBtn);
    mainScene.addView(settingsIcon);
    mainScene.show();
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
    client.setDataReadyCallback<AppController>(this, &AppController::noticeResponse);
    client.setCompletionCallback<AppController>(this, &AppController::noticeCompletion);
    client.post();
}

void AppController::showSettings()
{
    mainScene.hide();
    settingScn.show();
}

void AppController::showApp()
{
    settingScn.hide();
    mainScene.show();
}

void AppController::monoWakeFromReset()
{
    clkTimer.start();
    
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
    
}

void AppController::monoWakeFromSleep()
{
    IApplicationContext::Instance->DisplayController->setBrightness(0);
    pwrsave.undim();
    
    showApp();
    
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
    dataIcon.setForeground(mono::ui::View::StandardBackgroundColor);
    dataIcon.scheduleRepaint();
    
    uploadTask.reschedule(DateTime::now().addMinutes(uploadIntervalMins));
    //async(IApplicationContext::EnterSleepMode);
    //pwrsave.startDimTimer();
    pwrsave.dim();
}

void AppController::networkError()
{
    dataIcon.setForeground(mono::ui::View::StandardBackgroundColor);
    dataIcon.scheduleRepaint();
    
    pwrsave.dim();
    printf("Network error!\r\n");
    uploadTask.reschedule(DateTime::now().addMinutes(uploadIntervalMins));
}

uint16_t AppController::noticeBodyLength()
{

    int bodyLen = snprintf(0, 0, "token=%s&user=%s&title=%s&message=%s&url=",
                    PushoverTokenId, PushoverUserId,
                    levelNotice.Title(), levelNotice.Message());

    network::Url url = network::Url::Format("http://grapher.openmono.com/data/%s/%s/%s?title=Templog&timeformat=MM-DD HH:mm&yaxis=℃",
                    GrapherUserId, GrapherContainerId, GrapherSeriesId);

    return bodyLen+url.Length();
}

void AppController::noticeBodyData(char *data)
{
    network::Url url = network::Url::Format("http://grapher.openmono.com/data/%s/%s/%s?title=Templog&timeformat=MM-DD HH:mm&yaxis=℃",
                                GrapherUserId, GrapherContainerId, GrapherSeriesId);

    // +1 because body length does not include NULL terminator
    snprintf(data, noticeBodyLength()+1, "token=%s&user=%s&title=%s&message=%s&url=%s",
             PushoverTokenId, PushoverUserId,
             levelNotice.Title(), levelNotice.Message(), url());
}

void AppController::noticeResponse(const network::HttpClient::HttpResponseData &resp)
{
    printf("%s\r\n", resp.bodyChunk());
}

void AppController::noticeCompletion(network::INetworkRequest::CompletionEvent *)
{
    printf("Done sending notification!\r\n");
    dataIcon.setForeground(mono::ui::View::StandardBackgroundColor);
    dataIcon.scheduleRepaint();
    
    pwrsave.dim();
}

