#include "app_controller.h"
#include <ptmono30.h>

AppController::AppController() :
    tempTimer(1000),
    clkTimer(500),
    tempLbl(mono::geo::Rect(0,40,176,110),""),
    graph(109,109),
    timeLbl(mono::geo::Rect(0,89,176,15), "time")
{
    secInterval = 1;
    tempLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    tempLbl.setFont(PT_Mono_30);
    tempLbl.setText(mono::display::CloudsColor);
    tempLbl.show();
    
    timeLbl.setAlignment(mono::ui::TextLabelView::ALIGN_CENTER);
    timeLbl.show();
    
    getTemp(true);
    graph.show();

    //tempTimer.setCallback<AppController>(this, &AppController::getTemp);
    clkTimer.setCallback<AppController>(this, &AppController::updateClock);
}

void AppController::getTemp(bool firstRun)
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
    graph.setNextPoint(tempC);
    graph.scheduleRepaint();
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
    
    getTemp();
    
    if (fenced)
        power->setPowerFence(true);
    
    updateClock();
    tmpTask.reschedule(DateTime::now().addSeconds(secInterval));
}

void AppController::updateClock()
{
    timeLbl.setText(mono::DateTime::now().toTimeString());
}

void AppController::fillTempFilter(int temp)
{
    for(int i=0; i<this->filterLength; i++)
    {
        tempFilter[i] = temp;
    }
    filterPosition = 0;
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
    secInterval = 10;
}

void AppController::monoWakeFromSleep()
{
    IApplicationContext::Instance->DisplayController->setBrightness(0);
    pwrsave.undim();
    
    tempLbl.scheduleRepaint();
    graph.scheduleRepaint();
    timeLbl.scheduleRepaint();

    secInterval = 10;
    getTempTask();
}
