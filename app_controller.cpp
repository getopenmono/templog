#include "app_controller.h"
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

const MonoFont *mono::ui::TextLabelView::StandardTextFont = 0;
const GFXfont *mono::ui::TextLabelView::StandardGfxFont = &FreeSans9pt7b;

AppController::AppController() :
    clkTimer(999),
    tempLbl(mono::geo::Rect(10,10,156,60),""),
    graph(109,109),
    timeLbl(mono::geo::Rect(0,65,176,25), ""),
    settingsBtn(mono::geo::Rect(150,5,176-151,30), "i")
{
    secInterval = 164;
    
    tempLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    tempLbl.setFont(FreeSans18pt7b);
    tempLbl.setText(mono::display::CloudsColor);
    tempLbl.show();
    
    timeLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    timeLbl.setFont(FreeSans9pt7b);
    timeLbl.show();
    
    clkTimer.setCallback<AppController>(this, &AppController::updateClock);
    
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
    printf("Will connect and upload temp...\r\n");
    internet.setConnectCallback<AppController>(this, &AppController::uploadTempReady);
#ifdef WLAN_SSID
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
    printf("uploading temp...\r\n");
      client = network::HttpPostClient("http://grapher.openmono.com/autostamp/stoffera/templog/8", "Content-Type: application/x-www-form-urlencoded\r\n");
    //client = network::HttpPostClient("http://kong.ptype.dk/autostamp/stoffera/templog/2", "Content-Type: application/x-www-form-urlencoded\r\n");
    client.setBodyLengthCallback<AppController>(this, &AppController::postBodyLength);
    client.setBodyDataCallback<AppController>(this, &AppController::postBody);
    client.setCompletionCallback<AppController>(this, &AppController::tempUploadCompletion);

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
    
#ifdef WLAN_SSID
#ifdef WLAN_PASS
    uploadTask = ScheduledTask(DateTime::now().addMinutes(5));
    uploadTask.setRunInSleep(true);
    uploadTask.setTask<AppController>(this, &AppController::uploadTempAsync);
#endif
#endif
}

void AppController::monoWillGotoSleep()
{
    pwrsave.deactivate();
    IApplicationContext::Instance->DisplayController->setBrightness(0);
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
    uploadTask.reschedule(DateTime::now().addMinutes(30));
    //async(IApplicationContext::EnterSleepMode);
    //pwrsave.startDimTimer();
    pwrsave.dim();
}
