#include "graph_view.h"
#include <text_render.h>

using mono::geo::Rect;
using mono::String;

namespace {
	const uint8_t xMin = 1;
	const uint8_t xMax = 175;
}

GraphView::GraphView (uint8_t y_, uint8_t height_)
:
	View(Rect(xMin-1,y_,xMax-xMin+1,height_)),
	y(y_),
	height(height_),
	ixBegin(0),
	ixPastLastElement(0)
{
	static float d[176];
	data = d;
    secsBetweenSamples = 1;
    secsDirty = true;
}

void GraphView::setSecsBetweenPoints(int secs)
{
    secsDirty = secsBetweenSamples != secs;
    secsBetweenSamples = secs;
}

void GraphView::repaint ()
{
	maxData = 33.0;
	minData = 3.0;
    
    painter.setTextSize(1);
    
    String text = String::Format("%d",(int)maxData);
    Rect upr(viewRect.X(), viewRect.Y() + 1, viewRect.Width(), viewRect.Height());
    int cursor = upr.X();
    for (unsigned int x=0; x<text.Length(); x++) {
        painter.drawChar(cursor, upr.Y(), text[x]);
        cursor += 5;
    }
    
    text = String::Format("%d",(int)minData);
    mono::geo::Point lwr(viewRect.X()+5, viewRect.Y2() - 7 - 8);
    cursor = lwr.X();
    for (unsigned int x=0; x<text.Length(); x++) {
        painter.drawChar(cursor, lwr.Y(), text[x]);
        cursor += 5;
    }

    cursor = 1+viewRect.X();
    mono::String time = mono::DateTime::now().addSeconds(-secsBetweenSamples*175).toTimeString();
    for (unsigned int i=0; i<time.Length(); i++) {
        painter.drawChar(cursor, viewRect.Y2()-7, time[i]);
        cursor += 5;
    }
    
    time = mono::DateTime::now().toTimeString();
    cursor = viewRect.X2() - time.Length()*5;
    for (unsigned int i=0; i<time.Length(); i++) {
        painter.drawChar(cursor, viewRect.Y2()-7, time[i]);
        cursor += 5;
    }
    
	deletePoints();
	drawPoints();
	painter.setForegroundColor(mono::display::MidnightBlueColor);
    
    Rect border(viewRect.X()+11, viewRect.Y(), viewRect.Width() - 11, viewRect.Height() - 8);
	painter.drawRect(border);
}

void GraphView::deletePoints ()
{
	uint16_t x = 175;
	for (size_t i = ixPastLastElement-1; i > 0; --i)
	{
		deletePoint(x,data[i-1]);
		--x;
	}
	if (ixBegin == ixPastLastElement) for (size_t i = 176; i > ixBegin; --i)
	{
		if (x < 1) break;
		deletePoint(x,data[i-1]);
		--x;
	}
}

void GraphView::drawPoints ()
{
	uint16_t x = 175;
	for (size_t i = ixPastLastElement; i > 0; --i)
	{
		drawPoint(x,data[i-1]);
		--x;
	}
	if (ixBegin == ixPastLastElement) for (size_t i = 176; i > ixBegin; --i)
	{
		if (x < 1) break;
		drawPoint(x,data[i-1]);
		--x;
	}
}

void GraphView::deletePoint (uint8_t x, float value)
{
	if (value > maxData)
	{
		uint16_t point = convertPointToY(maxData);
		painter.drawPixel(x,point,true);
	}
	else if (value <= minData)
	{
		uint16_t point = convertPointToY(minData);
		painter.drawPixel(x,point,true);
	}
	else
	{
		uint16_t point = convertPointToY(value);
		painter.drawPixel(x,point,true);
	}
}

void GraphView::drawPoint (uint8_t x, float value)
{
	if (value > maxData)
	{
		uint16_t point = convertPointToY(maxData);
		painter.setForegroundColor(mono::display::RedColor);
		painter.drawPixel(x,point);
	}
	else if (value <= minData)
	{
		uint16_t point = convertPointToY(minData);
		painter.setForegroundColor(mono::display::RedColor);
		painter.drawPixel(x,point);
	}
	else
	{
		uint16_t point = convertPointToY(value);
		painter.setForegroundColor(mono::display::AlizarinColor);
		painter.drawPixel(x,point);
	}
}

uint16_t GraphView::convertPointToY (float point)
{
	float scaledY = (point - minData) / (maxData - minData + 1);
	float p = height + y - (scaledY * height);
	return p - 2;
}

void GraphView::setNextPoint (float point)
{
	if (ixBegin == 0 && ixPastLastElement != 176)
	{
		++ixPastLastElement;
	}
	else
	{
		++ixPastLastElement;
		++ixBegin;
		if (ixPastLastElement > 176) ixPastLastElement = 1;
		if (ixBegin >= 176) ixBegin = 0;
	}
	data[ixPastLastElement-1] = point;
}

void GraphView::updateMinMax (float point)
{
	if (minData > point) minData = point;
	if (maxData < point) maxData = point;
}
