#!/bin/bash
./build/earthcall_webgpu > logs/click_test.log 2>&1 &
APP_PID=$!
sleep 3

osascript -e '
tell application "System Events"
    tell process "earthcall_webgpu"
        set frontmost to true
        delay 1
        set p to position of window 1
        set s to size of window 1
        set centerX to (item 1 of p) + (item 1 of s) / 2
        set centerY to (item 2 of p) + (item 2 of s) / 2
        
        repeat 30 times
            click at {centerX, centerY}
            delay 0.03
        end repeat
    end tell
end tell
'

sleep 2
kill -9 $APP_PID
