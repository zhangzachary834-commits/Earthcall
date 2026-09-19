#!/bin/bash
./build/earthcall_webgpu > logs/click_test.log 2>&1 &
APP_PID=$!
sleep 2

# Load the synthesis studio via Python scripting? No, I can't.
# But I can wait for the app to start and just let it run.
kill -9 $APP_PID
