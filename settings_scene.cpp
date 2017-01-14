
#include "settings_scene.h"

SettingsScene::SettingsScene() :
    timeScaleLbl(Rect(10, 10, 156, 35), "Graph timescale:"),
    ssidLbl(Rect(10, 135, 176, 35), "Wifi: -"),
    hrs4(Rect(40        , 5+35, 50, 35), "4 hr."),
    hrs8(Rect(40+50+5   , 5+35, 50, 35), "8 hr."),
    hrs24(Rect(40       , 5+75, 50, 35), "24 hr."),
    hrs48(Rect(40+50+5  , 5+75, 50, 35), "48 hr."),
    connectBtn(mono::geo::Rect(20, 160, 176-40, 35), "Connect")
{
    
}

void SettingsScene::repaint()
{
    // paint background
    painter.setBackgroundColor(View::StandardBackgroundColor);
    painter.drawFillRect(0,0,DisplayWidth(),DisplayHeight(), true);
    
    timeScaleLbl.scheduleRepaint();
    ssidLbl.scheduleRepaint();
    hrs4.scheduleRepaint();
    hrs8.scheduleRepaint();
    hrs24.scheduleRepaint();
    hrs48.scheduleRepaint();
    connectBtn.scheduleRepaint();
}

void SettingsScene::show()
{
    mono::ui::View::show();
    timeScaleLbl.show();
    ssidLbl.show();
    hrs4.show();
    hrs8.show();
    hrs24.show();
    hrs48.show();
    connectBtn.show();
}

void SettingsScene::hide()
{
    mono::ui::View::hide();
    timeScaleLbl.hide();
    ssidLbl.hide();
    hrs4.hide();
    hrs8.hide();
    hrs24.hide();
    hrs48.hide();
    connectBtn.hide();
    
    scheduleRepaint();
}
