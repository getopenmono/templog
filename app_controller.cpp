#include "app_controller.h"
#include <ptmono30.h>

AppController::AppController() :
    clkTimer(999),
    tempLbl(mono::geo::Rect(0,40,176,110),""),
    graph(109,109),
    timeLbl(mono::geo::Rect(0,89,176,15), "time"),
    settingsBtn(mono::geo::Rect(150,5,176-151,30), "i")
{
    secInterval = 164;
    
    tempLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    tempLbl.setFont(PT_Mono_30);
    tempLbl.setText(mono::display::CloudsColor);
    tempLbl.show();
    
    timeLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    timeLbl.show();
    
    clkTimer.setCallback<AppController>(this, &AppController::updateClock);
    
#ifdef MONO_COMPILE_TIMESTAMP
    DateTime::setSystemDateTime(DateTime::fromISO8601(MONO_COMPILE_TIMESTAMP));
#endif
    
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
