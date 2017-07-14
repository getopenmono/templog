
#include "settings_scene.h"
#include <Fonts/FreeSans9pt7b.h>

SettingsScene::SettingsScene() :
    timeScaleLbl(Rect(10, 10, 156, 35), "Graph timescale:"),
    ssidLbl(Rect(10, 125, 176, 25), "Wifi: -"),
    hrs4(Rect(40        , 5+35, 50, 35), "4 hr"),
    hrs8(Rect(40+50+5   , 5+35, 50, 35), "8 hr"),
    hrs24(Rect(40       , 5+75, 50, 35), "24hr"),
    hrs48(Rect(40+50+5  , 5+75, 50, 35), "48hr"),
    connectBtn(mono::geo::Rect(20, 150, 176-40, 35), "Connect"),
    backBtn(Rect(10,220-40, 80, 35), "Back")
{
    timeScaleLbl.setFont(FreeSans9pt7b);
    ssidLbl.setFont(FreeSans9pt7b);
    
    hrs4.setClickCallback<SettingsScene>(this, &SettingsScene::hrs4Call);
    hrs8.setClickCallback<SettingsScene>(this, &SettingsScene::hrs8Call);
    hrs24.setClickCallback<SettingsScene>(this, &SettingsScene::hrs24Call);
    hrs48.setClickCallback<SettingsScene>(this, &SettingsScene::hrs48Call);
    connectBtn.setClickCallback<SettingsScene>(this, &SettingsScene::cnctCall);
    backBtn.setClickCallback(this, &SettingsScene::requestDismiss);
    
    this->setShowCallback(this, &SettingsScene::didShow);
    this->setHideCallback(this, &SettingsScene::didHide);
    
#ifdef WLAN_SSID
#ifdef WLAN_PASS
    ssidLbl.setText("Wifi: " WLAN_SSID);
#endif
#endif
    
    addView(timeScaleLbl);
    addView(ssidLbl);
    addView(hrs4);
    addView(hrs8);
    addView(hrs24);
    addView(hrs48);
    //addView(connectBtn);
    addView(backBtn);
}

void SettingsScene::didShow(const SceneController&)
{
    
}

void SettingsScene::didHide(const SceneController&)
{
    
}

void SettingsScene::requestDismiss()
{
    dismissHandler.call();
}

//void SettingsScene::repaint()
//{
//    // paint background
//    painter.setBackgroundColor(View::StandardBackgroundColor);
//    painter.drawFillRect(0,0,DisplayWidth(),DisplayHeight(), true);
//    
//    timeScaleLbl.scheduleRepaint();
//    ssidLbl.scheduleRepaint();
//    hrs4.scheduleRepaint();
//    hrs8.scheduleRepaint();
//    hrs24.scheduleRepaint();
//    hrs48.scheduleRepaint();
//    connectBtn.scheduleRepaint();
//    
//    
//}
//
//void SettingsScene::show()
//{
//    mono::ui::View::show();
//    timeScaleLbl.show();
//    ssidLbl.show();
//    hrs4.show();
//    hrs8.show();
//    hrs24.show();
//    hrs48.show();
//    connectBtn.show();
//}
//
//void SettingsScene::hide()
//{
//    mono::ui::View::hide();
//    timeScaleLbl.hide();
//    ssidLbl.hide();
//    hrs4.hide();
//    hrs8.hide();
//    hrs24.hide();
//    hrs48.hide();
//    connectBtn.hide();
//    
//    scheduleRepaint();
//    
//    async<mbed::FunctionPointer>(&dismissHandler, &mbed::FunctionPointer::call);
//}

void SettingsScene::hrs4Call() { timescaleHandler.call(4); }
void SettingsScene::hrs8Call() { timescaleHandler.call(8); }
void SettingsScene::hrs24Call() { timescaleHandler.call(24); }
void SettingsScene::hrs48Call() { timescaleHandler.call(48); }
void SettingsScene::cnctCall() { connectHandler.call(); }
