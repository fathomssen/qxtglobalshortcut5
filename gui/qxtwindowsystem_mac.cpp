#ifndef QXTWINDOWSYSTEM_MAC_CPP
#define QXTWINDOWSYSTEM_MAC_CPP

#include "qxtwindowsystem.h"

#include <Carbon/Carbon.h>
#include <MacTypes.h>

#include <map>

using namespace std;

using Layer = int;
using WinIdLayers = multimap<Layer, WId>;

#define WINDOW_NOT_FOUND (WId)(0)
#define INVALID_LAYER (Layer)(-2147483626)

WId getWindowId( CFDictionaryRef winInfo )
{
    // Get the window number - this property is always available
    CFNumberRef winNum = (CFNumberRef)CFDictionaryGetValue( winInfo, kCGWindowNumber );

    // Convert window number to WId; skip on error
    WId winId;
    if( !CFNumberGetValue(winNum, kCFNumberSInt64Type, &winId) )
        return WINDOW_NOT_FOUND;

    return winId;
}

Layer getWindowLayer( CFDictionaryRef winInfo )
{
    // Get the window layer - this property is always available
    CFNumberRef winLayer = (CFNumberRef)CFDictionaryGetValue( winInfo, kCGWindowLayer );

    // Convert window number to WId; skip on error
    Layer layer;
    if( !CFNumberGetValue(winLayer, kCFNumberIntType, &layer) )
        return INVALID_LAYER;

    return layer;
}

QRect getWindowGeometry( CFDictionaryRef winInfo )
{
    // Get the window bounds - this property is always available
    CFDictionaryRef winBounds = (CFDictionaryRef)CFDictionaryGetValue( winInfo, kCGWindowBounds );

    // Convert window number to WId; skip on error
    CGRect rect;
    if( !CGRectMakeWithDictionaryRepresentation(winBounds, &rect) )
        return QRect();

    return QRect( rect.origin.x, rect.origin.y, rect.size.width, rect.size.height );
}

WinIdLayers getVisibleWindows()
{
    // This should mimic the behaviour of the Windows version closest
    CGWindowListOption winOptions = kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements;

    // Get information about all windows matching winOptions
    CFArrayRef winInfos = CGWindowListCopyWindowInfo( winOptions, kCGNullWindowID );

    WinIdLayers windows;
    for( int i = 0; i < CFArrayGetCount(winInfos); ++i )
    {
        CFDictionaryRef winInfo = (CFDictionaryRef)CFArrayGetValueAtIndex( winInfos, i );

        WId winId = getWindowId( winInfo );
        if( winId != WINDOW_NOT_FOUND )
            windows.insert( make_pair(getWindowLayer(winInfo), winId) );
    }

    return windows;
}

WindowList QxtWindowSystem::windows()
{
    WindowList ret;
    for( const auto win : getVisibleWindows() )
        ret.push_back( win.second );

    return ret;
}

WId QxtWindowSystem::activeWindow()
{
    for( const auto win : getVisibleWindows() )
    {
        // Use the first window ID with layer 0
        if( win.first == 0 )
            return win.second;
    }

    return WINDOW_NOT_FOUND;
}

QString QxtWindowSystem::windowTitle( WId winId )
{
    CFArrayRef winInfos = CGWindowListCopyWindowInfo( kCGWindowListOptionIncludingWindow, winId );

    if( CFArrayGetCount(winInfos) > 0 )
    {
        CFDictionaryRef winInfo = (CFDictionaryRef)CFArrayGetValueAtIndex( winInfos, 0 );
        CFStringRef winTitle = (CFStringRef)CFDictionaryGetValue( winInfo, kCGWindowName );

        if( QString::fromCFString(winTitle) == "Item-0" )
            winTitle = (CFStringRef)CFDictionaryGetValue( winInfo, kCGWindowOwnerName );

        return QString::fromCFString( winTitle );
    }

    return QString();
}

QRect QxtWindowSystem::windowGeometry( WId winId )
{
    CFArrayRef winInfos = CGWindowListCopyWindowInfo( kCGWindowListOptionIncludingWindow, winId );

    if( CFArrayGetCount(winInfos) > 0 )
    {
        CFDictionaryRef winInfo = (CFDictionaryRef)CFArrayGetValueAtIndex( winInfos, 0 );
        return getWindowGeometry( winInfo );
    }

    return QRect();
}

// Source: http://developer.apple.com/library/mac/#documentation/Carbon/Reference/QuartzEventServicesRef/Reference/reference.html
uint QxtWindowSystem::idleTime()
{
    // CGEventSourceSecondsSinceLastEventType returns time in seconds as a double
    double idle = 1000 * ::CGEventSourceSecondsSinceLastEventType(kCGEventSourceStateCombinedSessionState, kCGAnyInputEventType);
    return (uint)idle;
}


// these are copied from X11 implementation
WId QxtWindowSystem::findWindow( const QString& title )
{
    for( const auto winId : windows() )
    {
        if( windowTitle(winId) == title )
            return winId;
    }

    return WINDOW_NOT_FOUND;
}

WId QxtWindowSystem::windowAt( const QPoint& pos )
{
    for( const auto winId : windows() )
    {
        if( windowGeometry(winId).contains(pos) )
            return winId;
    }

    return WINDOW_NOT_FOUND;
}

#endif // QXTWINDOWSYSTEM_MAC_CPP
